#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;
in layout(location = 3) vec3 indexed_tangents;
in layout(location = 4) vec3 indexed_bitangents;


uniform layout(location = 3) mat4 M;
uniform layout(location = 4) mat4 P;
uniform layout(location = 8) mat4 V;
uniform layout(location = 5) mat3 normal_matrix;
uniform layout(location = 6) int do_textures;
uniform layout(location = 7) int is_2d;
uniform layout(location = 10) int is_skybox;

//TODO: multiply normal_matrix with TBA matrix

out layout(location = 0) vec3 pos_out;
out layout(location = 1) vec3 normal_out;
out layout(location = 2) vec2 textureCoordinates_out;
out layout(location = 3) mat3 TBN;

void main()
{   
    //TBN is mostly stolen from the tutorial
    vec3 vertexNormal_cameraspace = normal_matrix * normalize(normal_in);
    vec3 vertexTangent_cameraspace = normal_matrix * normalize(indexed_tangents);
    vec3 vertexBitangent_cameraspace = normal_matrix * normalize(indexed_bitangents);

    TBN = transpose(mat3(
        vertexTangent_cameraspace,
        vertexBitangent_cameraspace,
        vertexNormal_cameraspace
    ));

    if (is_skybox != 0)  {
        pos_out = position;
    } else {
        pos_out = (V * M * vec4(position, 1.0f)).xyz;
    }

    normal_out = normalize(normal_matrix * normal_in);
    textureCoordinates_out = textureCoordinates_in;
    if(is_2d == 0){
        gl_Position = P * V * M * vec4(position, 1.0f);
    } else {
        gl_Position = M * vec4(position, 1.0f);
    }
}
