#version 430 core

in layout(location = 0) vec3 pos; // mv
in layout(location = 1) vec3 normal;
in layout(location = 2) vec2 textureCoordinates;
in layout(location = 3) mat3 TBN;

out vec4 color;

struct LightInfo {
    vec3 position;
    vec3 color;
};
uniform LightInfo light_info[3];
//uniform vec3 lights[3];//In gamelogic store positions of all lights and put them in a uniform after we are done updating their nodes, this one is now replaced with light_info
uniform vec3 ball_pos;
uniform layout(location = 6) int do_textures;
uniform layout(location = 7) int is_2d;
uniform layout(location = 9) int do_roughness;
uniform layout(location = 10) int is_skybox;
uniform layout(location = 11) int do_metal_roughness;
uniform layout(location = 12) int dynamicCube;


layout(binding = 0) uniform sampler2D diffuseTexture;
layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D roughnessMap;
layout(binding = 3) uniform samplerCube cubeMap;
layout(binding = 4) uniform sampler2D metalRoughnessMap;
layout(binding = 5) uniform samplerCube dynamicCubeMap;


vec4 diffuse_texture_color = texture(diffuseTexture, textureCoordinates);
vec3 normal_texture_color = TBN*(texture(normalMap, textureCoordinates).xyz*2-1);
vec4 roughness_texture = texture(roughnessMap, textureCoordinates);
vec4 metal_roughness_texture = texture(metalRoughnessMap, textureCoordinates);


float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }

vec3 reject(vec3 from, vec3 onto){
    return from - onto*dot(from, onto)/dot(onto, onto);
}



const vec3 camerapos = vec3(0.0);
vec3 surface_to_eye = normalize(camerapos - pos);
const vec3 ambient_color = vec3(0.2, 0.2, 0.2);

const float l_a = 0.001;
const float l_b = 0.001;
const float l_c = 0.001;
const float ball_radius = 3.0;
const float soft_shadow_ball_radius = 5.0;

void main()
{
    vec3 normalized_normal;
    if (is_2d == 0 && do_textures != 0){ // activates if we use textures and don't do 2d model. Perhaps I should have made a uniform solely to signify we have a normal map, but I am lazy and will implement it when it is necessary
        normalized_normal = normalize(vec3(normal_texture_color)); //Replace normal with normal texture if it exists
    }
    else{
        normalized_normal = normalize(normal); 
    }

    float sharpness_factor;
    if (do_roughness != 0){ // Activates if we have a roughness map
        sharpness_factor = 5/(length(roughness_texture)*length(roughness_texture));
    }
    else{
        if(do_metal_roughness != 0){
            sharpness_factor = 5/(length(metal_roughness_texture.g)*length(metal_roughness_texture.g));
        }
        else{
            sharpness_factor = 32;
        }
    }

    if(is_2d == 0){ // is 3d
        if (is_skybox == 0){
            vec3 diffuse_out = vec3(0,0,0);
            vec3 specular_out = vec3(0,0,0);

            vec3 frag_to_ball_center = ball_pos-pos;
            float hardening = 1;

            for(int i = 0; i < 1; i++) { // Just process one of the lights (just loop 3 again to get the other ones)
                vec3 frag_to_light = light_info[i].position - pos;
                bool shadow = (length(reject(frag_to_ball_center, frag_to_light)) < ball_radius) 
                                && (length(frag_to_light) > (length(frag_to_ball_center))+ball_radius) && (dot(frag_to_light,frag_to_ball_center) > 0);

                bool soft_shadow = (length(reject(frag_to_ball_center, frag_to_light)) < soft_shadow_ball_radius) 
                                && (length(frag_to_light) > (length(frag_to_ball_center))+soft_shadow_ball_radius) && (dot(frag_to_light,frag_to_ball_center) > 0);

                if (!shadow){
                    vec3 light_dir = normalize(frag_to_light);
                    float light_to_fragment_distance = length(pos-light_info[i].position);
                    float L = 1/(l_a + light_to_fragment_distance*l_b + pow(light_to_fragment_distance, 2)*l_c); //attenuation

                    //Check for soft shadow
                    if(soft_shadow){
                        hardening = (length(reject(frag_to_ball_center, frag_to_light)) - ball_radius)/2; //Soft shadows based on how close we are to actual shadow. I normalized it between 0 and 1 where 0 is no light and 1 is all light.
                    }
                    else{
                        hardening = 1;
                    }

                    // See if we need to use diffuse colors of texture
                    if (do_textures != 0){
                        diffuse_out += max(0.0, dot(normalized_normal, light_dir))*L * light_info[i].color*hardening * vec3(diffuse_texture_color);
                    }
                    else{
                        diffuse_out += max(0.0, dot(normalized_normal, light_dir))*L * light_info[i].color*hardening;
                    }
                    
                    // Good ol' specular
                    specular_out += pow(max(0.0, dot(reflect(-light_dir, normalized_normal), surface_to_eye)), sharpness_factor)*L* light_info[i].color*hardening;
                }
                
            }

            if(do_metal_roughness != 0){ // Right now metal roughness does not affect anything but roughness, which is whay both results of the if are the same
                vec3 I = normalize(pos - camerapos);
                //vec3 R = reflect(I, normalized_normal);
                vec3 R = reflect(I, normalize(normal)); //When cat only use regular normal, and not TBN normal
                if (dynamicCube == 1){
                    color = vec4((texture(dynamicCubeMap, R).rgb*diffuse_texture_color.rgb + diffuse_out + specular_out + dither(textureCoordinates)), 1.0); 
                }
                else{
                    color = vec4((texture(cubeMap, R).rgb*diffuse_texture_color.rgb + diffuse_out + specular_out + dither(textureCoordinates)), 1.0); 

                }
            }else {
                float refraction_ratio = 1.00/1.33;
                vec3 I = normalize(pos - camerapos);
                //vec3 R = reflect(I, normalized_normal);
                //vec3 R = reflect(I, normalize(normal));
                vec3 R = refract(I, normalized_normal, refraction_ratio);
                //color = vec4((diffuse_out + specular_out + ambient_color + dither(textureCoordinates)), 1.0);
                color = vec4((texture(cubeMap, R).rgb + diffuse_out + specular_out + dither(textureCoordinates)), 1.0); 
                //color = vec4((diffuse_out + specular_out + ambient_color + dither(textureCoordinates)), 1.0);
            }
        }
        else{
            if (dynamicCube == 1){
                color = texture(dynamicCubeMap, normalize(pos - camerapos)); // The dynamic box (only drawn at skybox for debug purposes)
            }
            else {
                color = texture(cubeMap, normalize(pos - camerapos)); // The sky box
            }
        }
    }
    else{ // is_2d
        color = diffuse_texture_color;
    }
}