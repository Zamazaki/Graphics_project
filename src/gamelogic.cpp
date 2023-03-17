#include <chrono>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <SFML/Audio/SoundBuffer.hpp>
#include <utilities/shader.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utilities/timeutils.h>
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include <SFML/Audio/Sound.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include "gamelogic.h"
#include "sceneGraph.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "utilities/imageLoader.hpp"
#include "utilities/glfont.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

enum KeyFrameAction {
    BOTTOM, TOP
};

#include <timestamps.h>

double padPositionX = 0;
double padPositionZ = 0;

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

SceneNode* rootNode;
SceneNode* charTextureNode;
SceneNode* boxNode;
SceneNode* ballNode;
SceneNode* padNode;
SceneNode* skyboxNode;
SceneNode* catNode;

double ballRadius = 3.0f;

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
sf::Sound* sound;

const glm::vec3 boxDimensions(180, 90, 90);
const glm::vec3 padDimensions(30, 3, 40);

glm::vec3 ballPosition(0, ballRadius + padDimensions.y, boxDimensions.z / 2);
glm::vec3 ballDirection(1, 1, 0.2f);

CommandLineOptions options;

bool hasStarted        = false;
bool hasLost           = false;
bool jumpedToNextFrame = false;
bool isPaused          = false;

bool mouseLeftPressed   = false;
bool mouseLeftReleased  = false;
bool mouseRightPressed  = false;
bool mouseRightReleased = false;

// Modify if you want the music to start further on in the track. Measured in seconds.
const float debug_startTime = 0;
double totalElapsedTime = debug_startTime;
double gameElapsedTime = debug_startTime;

double mouseSensitivity = 1.0;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;
void mouseCallback(GLFWwindow* window, double x, double y) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    double deltaX = x - lastMouseX;
    double deltaY = y - lastMouseY;

    padPositionX -= mouseSensitivity * deltaX / windowWidth;
    padPositionZ -= mouseSensitivity * deltaY / windowHeight;

    if (padPositionX > 1) padPositionX = 1;
    if (padPositionX < 0) padPositionX = 0;
    if (padPositionZ > 1) padPositionZ = 1;
    if (padPositionZ < 0) padPositionZ = 0;

    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
}

// A few lines to help you if you've never used c++ structs
struct LightSource {
    SceneNode *lightNode;
    glm::vec3 lightColor;
};
LightSource lightSources[3];


//For general 2d textures
void uploadTexture(GLuint *unbound_int, PNGImage& image){
    glGenTextures(1, unbound_int);
    glBindTexture(GL_TEXTURE_2D, *unbound_int);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA,
        image.width, image.height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data()); //GL_RGBA8
    glGenerateMipmap(GL_TEXTURE_2D);
}


//For tiny object loader, got it from Odd Erik
Mesh loadObj(std::string filename){
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string error;
    std::string warning;
    tinyobj::LoadObj(&attributes, &shapes, &materials, &warning, &error, filename.c_str(), nullptr, false);

    Mesh m;
    if (shapes.size() > 1){
        std::cerr << "Unsupported obj format: more than one mesh in file" << std::endl;
        return m;
    }
    if (shapes.size() == 0){
        std::cerr << error << std::endl;
        return m;
    }
    const auto &shape = shapes.front();
    std::cout << "Loaded object " << shape.name << " from file " << filename << std::endl;
    const auto &tmesh = shape.mesh;
    for (const auto i : tmesh.indices){
        m.indices.push_back(m.indices.size());
        glm::vec3 pos;
        pos.x = attributes.vertices.at(3*i.vertex_index);
        pos.y = attributes.vertices.at(3*i.vertex_index+1);
        pos.z = attributes.vertices.at(3*i.vertex_index+2);
        m.vertices.push_back(pos);
        glm::vec3 normal;
        normal.x = attributes.normals.at(3*i.normal_index + 0);
        normal.y = attributes.normals.at(3*i.normal_index + 1);
        normal.z = attributes.normals.at(3*i.normal_index + 2);
        m.normals.push_back(normal);
        glm::vec2 uv;
        uv.x = attributes.texcoords.at(2*i.texcoord_index + 0);
        uv.y = attributes.texcoords.at(2*i.texcoord_index + 1);
        m.textureCoordinates.push_back(uv);

    }

    return m;
}

void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {
   
    buffer = new sf::SoundBuffer();
    if (!buffer->loadFromFile("../res/Hall of the Mountain King.ogg")) {
        return;
    }

    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);

    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();

    // Create meshes
    Mesh pad = cube(padDimensions, glm::vec2(30, 40), true);
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh sphere = generateSphere(1.0, 40, 40);
    Mesh cat = loadObj("../res/catlucky.obj");
    Mesh box_sky = cube(boxDimensions, glm::vec2(100), true, true);

    // Fill buffers
    unsigned int ballVAO = generateBuffer(sphere);
    unsigned int boxVAO  = generateBuffer(box);
    unsigned int padVAO  = generateBuffer(pad);
    unsigned int skyboxVAO = generateBuffer(box_sky);
    unsigned int catVAO = generateBuffer(cat);

    // Construct scene
    rootNode = createSceneNode(GEOMETRY);
    charTextureNode = createSceneNode(GEOMETRY_2D);
    boxNode  = createSceneNode(GEOMETRY_NORMAL_MAPPED);
    padNode  = createSceneNode(GEOMETRY);
    ballNode = createSceneNode(GEOMETRY);
    skyboxNode = createSceneNode(GEOMETRY);
    catNode  = createSceneNode(GEOMETRY_NORMAL_MAPPED);

    lightSources[0].lightNode = createSceneNode(POINT_LIGHT);
    lightSources[1].lightNode = createSceneNode(POINT_LIGHT);
    lightSources[2].lightNode = createSceneNode(POINT_LIGHT);
    
    lightSources[0].lightNode->lightID = 0;
    lightSources[1].lightNode->lightID = 1;
    lightSources[2].lightNode->lightID = 2;

    lightSources[0].lightColor = glm::vec3(1.0, 1.0, 1.0);
    lightSources[1].lightColor = glm::vec3(0.0, 1.0, 0.0);
    lightSources[2].lightColor = glm::vec3(0.0, 0.0, 1.0);

    lightSources[0].lightNode->position = glm::vec3(-10.0, 0.0, 5.0);
    lightSources[1].lightNode->position = glm::vec3( 0.0, 0.0, 5.0);
    lightSources[2].lightNode->position = glm::vec3( 10.0, 0.0, 5.0);

    rootNode->children.push_back(skyboxNode);
    //rootNode->children.push_back(boxNode);
    rootNode->children.push_back(padNode);
    rootNode->children.push_back(ballNode);
    //rootNode->children.push_back(charTextureNode);
    padNode->children.push_back(catNode);
    //rootNode->children.push_back(boxNode);

    addChild(padNode, lightSources[1].lightNode);
    //addChild(ballNode, lightSources[1].lightNode);
    //addChild(boxNode, lightSources[2].lightNode);

    //addChild(rootNode, lightSources[0].lightNode);
    addChild(rootNode, lightSources[1].lightNode);
    addChild(rootNode, lightSources[2].lightNode);


    boxNode->vertexArrayObjectID  = boxVAO;
    boxNode->VAOIndexCount        = box.indices.size();

    padNode->vertexArrayObjectID  = padVAO;
    padNode->VAOIndexCount        = pad.indices.size();

    ballNode->vertexArrayObjectID = ballVAO;
    ballNode->VAOIndexCount       = sphere.indices.size();

    catNode->vertexArrayObjectID  = catVAO;
    catNode->VAOIndexCount        = cat.indices.size();
    catNode->scale                = glm::vec3(2.5);
    catNode->position             = glm::vec3(-25.0, 0.0, 0.0);
    

    // Texture time
    PNGImage charmap =  loadPNGFile("../res/textures/charmap.png");
    GLuint charmap_id;

    uploadTexture(&charmap_id, charmap);
    Mesh charmapMesh = generateTextGeometryBuffer("Fisk", 39/29, 29);
    unsigned int charmapVAO = generateBuffer(charmapMesh);

    charTextureNode->vertexArrayObjectID = charmapVAO;
    charTextureNode->VAOIndexCount = charmapMesh.indices.size();
    charTextureNode->position = glm::vec3( 0.0, 0.0, 0.0);
    charTextureNode->scale = glm::vec3(0.12); // The texture was appearantly a bit big
    charTextureNode->textureID = charmap_id;


    // Texture time, but now with normals and such
    PNGImage diffuse_bricks =  loadPNGFile("../res/textures/Brick03_col.png");
    GLuint diffuse_bricks_id;
    uploadTexture(&diffuse_bricks_id, diffuse_bricks);
    boxNode->textureID = diffuse_bricks_id;

    PNGImage normal_bricks =  loadPNGFile("../res/textures/Brick03_nrm.png");
    GLuint normal_bricks_id;
    uploadTexture(&normal_bricks_id, normal_bricks);
    boxNode->normalMapTextureID = normal_bricks_id;

    PNGImage rough_bricks =  loadPNGFile("../res/textures/Brick03_rgh.png");
    GLuint rough_bricks_id;
    uploadTexture(&rough_bricks_id, rough_bricks);
    boxNode->roughnessMapID = rough_bricks_id;

    // Cat texture time
    PNGImage diffuse_cat =  loadPNGFile("../res/textures/cat_lucky/textures/maneki_white_baseColor_gold.png");
    GLuint diffuse_cat_id;
    uploadTexture(&diffuse_cat_id, diffuse_cat);
    catNode->textureID = diffuse_cat_id;

    PNGImage normal_cat =  loadPNGFile("../res/textures/cat_lucky/textures/maneki_white_normal.png");
    GLuint normal_cat_id;
    uploadTexture(&normal_cat_id, normal_cat);
    catNode->normalMapTextureID = normal_cat_id;

    /*PNGImage rough_cat =  loadPNGFile("../res/textures/Brick03_rgh.png");
    GLuint rough_cat_id;
    uploadTexture(&rough_cat_id, rough_cat);
    catNode->roughnessMapID = rough_cat_id;*/

    // I will attempt to construct my skybox here
    skyboxNode->vertexArrayObjectID = skyboxVAO;
    skyboxNode->VAOIndexCount       = box_sky.indices.size(); //skyboxIndices.size();

    std::vector<std::string> skyboxFaces {
        "../res/textures/cubemap/negx.jpg", //left
        "../res/textures/cubemap/posx.jpg", //right
        "../res/textures/cubemap/posy.jpg", //top
        "../res/textures/cubemap/negy.jpg", //bottom
        "../res/textures/cubemap/negz.jpg", //front
        "../res/textures/cubemap/posz.jpg", //back
    };

    GLuint skyboxTextureID;
    loadCubeMap(&skyboxTextureID, skyboxFaces);
    skyboxNode->textureID = skyboxTextureID;
    skyboxNode->isSkybox = true;



    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}

// Global boys that let's us seperate MVP
glm::mat4 projection;
glm::mat4 view;

void updateFrame(GLFWwindow* window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();

    const float ballBottomY = boxNode->position.y - (boxDimensions.y/2) + ballRadius + padDimensions.y;
    const float ballTopY    = boxNode->position.y + (boxDimensions.y/2) - ballRadius;
    const float BallVerticalTravelDistance = ballTopY - ballBottomY;

    const float cameraWallOffset = 30; // Arbitrary addition to prevent ball from going too much into camera

    const float ballMinX = boxNode->position.x - (boxDimensions.x/2) + ballRadius;
    const float ballMaxX = boxNode->position.x + (boxDimensions.x/2) - ballRadius;
    const float ballMinZ = boxNode->position.z - (boxDimensions.z/2) + ballRadius;
    const float ballMaxZ = boxNode->position.z + (boxDimensions.z/2) - ballRadius - cameraWallOffset;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
        mouseLeftPressed = true;
        mouseLeftReleased = false;
    } else {
        mouseLeftReleased = mouseLeftPressed;
        mouseLeftPressed = false;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2)) {
        mouseRightPressed = true;
        mouseRightReleased = false;
    } else {
        mouseRightReleased = mouseRightPressed;
        mouseRightPressed = false;
    }

    if(!hasStarted) {
        if (mouseLeftPressed) {
            if (options.enableMusic) {
                sound = new sf::Sound();
                sound->setBuffer(*buffer);
                sf::Time startTime = sf::seconds(debug_startTime);
                sound->setPlayingOffset(startTime);
                sound->play();
            }
            totalElapsedTime = debug_startTime;
            gameElapsedTime = debug_startTime;
            hasStarted = true;
        }

        ballPosition.x = ballMinX + (1 - padPositionX) * (ballMaxX - ballMinX);
        ballPosition.y = ballBottomY;
        ballPosition.z = ballMinZ + (1 - padPositionZ) * ((ballMaxZ+cameraWallOffset) - ballMinZ);
    } else {
        totalElapsedTime += timeDelta;
        if(hasLost) {
            if (mouseLeftReleased) {
                hasLost = false;
                hasStarted = false;
                currentKeyFrame = 0;
                previousKeyFrame = 0;
            }
        } else if (isPaused) {
            if (mouseRightReleased) {
                isPaused = false;
                if (options.enableMusic) {
                    sound->play();
                }
            }
        } else {
            gameElapsedTime += timeDelta;
            if (mouseRightReleased) {
                isPaused = true;
                if (options.enableMusic) {
                    sound->pause();
                }
            }
            // Get the timing for the beat of the song
            for (unsigned int i = currentKeyFrame; i < keyFrameTimeStamps.size(); i++) {
                if (gameElapsedTime < keyFrameTimeStamps.at(i)) {
                    continue;
                }
                currentKeyFrame = i;
            }

            jumpedToNextFrame = currentKeyFrame != previousKeyFrame;
            previousKeyFrame = currentKeyFrame;

            double frameStart = keyFrameTimeStamps.at(currentKeyFrame);
            double frameEnd = keyFrameTimeStamps.at(currentKeyFrame + 1); // Assumes last keyframe at infinity

            double elapsedTimeInFrame = gameElapsedTime - frameStart;
            double frameDuration = frameEnd - frameStart;
            double fractionFrameComplete = elapsedTimeInFrame / frameDuration;

            double ballYCoord;

            KeyFrameAction currentOrigin = keyFrameDirections.at(currentKeyFrame);
            KeyFrameAction currentDestination = keyFrameDirections.at(currentKeyFrame + 1);

            // Synchronize ball with music
            if (currentOrigin == BOTTOM && currentDestination == BOTTOM) {
                ballYCoord = ballBottomY;
            } else if (currentOrigin == TOP && currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance;
            } else if (currentDestination == BOTTOM) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * (1 - fractionFrameComplete);
            } else if (currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * fractionFrameComplete;
            }

            // Make ball move
            const float ballSpeed = 60.0f;
            ballPosition.x += timeDelta * ballSpeed * ballDirection.x;
            ballPosition.y = ballYCoord;
            ballPosition.z += timeDelta * ballSpeed * ballDirection.z;

            // Make ball bounce
            if (ballPosition.x < ballMinX) {
                ballPosition.x = ballMinX;
                ballDirection.x *= -1;
            } else if (ballPosition.x > ballMaxX) {
                ballPosition.x = ballMaxX;
                ballDirection.x *= -1;
            }
            if (ballPosition.z < ballMinZ) {
                ballPosition.z = ballMinZ;
                ballDirection.z *= -1;
            } else if (ballPosition.z > ballMaxZ) {
                ballPosition.z = ballMaxZ;
                ballDirection.z *= -1;
            }

            if(options.enableAutoplay) {
                padPositionX = 1-(ballPosition.x - ballMinX) / (ballMaxX - ballMinX);
                padPositionZ = 1-(ballPosition.z - ballMinZ) / ((ballMaxZ+cameraWallOffset) - ballMinZ);
            }

            // Check if the ball is hitting the pad when the ball is at the bottom.
            // If not, you just lost the game! (hehe)
            if (jumpedToNextFrame && currentOrigin == BOTTOM && currentDestination == TOP) {
                double padLeftX  = boxNode->position.x - (boxDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x);
                double padRightX = padLeftX + padDimensions.x;
                double padFrontZ = boxNode->position.z - (boxDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z);
                double padBackZ  = padFrontZ + padDimensions.z;

                if (   ballPosition.x < padLeftX
                    || ballPosition.x > padRightX
                    || ballPosition.z < padFrontZ
                    || ballPosition.z > padBackZ
                ) {
                    hasLost = true;
                    if (options.enableMusic) {
                        sound->stop();
                        delete sound;
                    }
                }
            }
        }
    }

    projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);

    glm::vec3 cameraPosition = glm::vec3(0, 2, -20);


    // rotate skybox lol
    skyboxNode->rotation.y += timeDelta / 10;


    //glUniform3fv(6, 1, glm::value_ptr(cameraPosition)); // Send camera position to fragement shader

    // Some math to make the camera move in a nice way
    float lookRotation = -0.6 / (1 + exp(-5 * (padPositionX-0.5))) + 0.3;
    glm::mat4 cameraTransform =
                    glm::rotate(0.3f + 0.2f * float(-padPositionZ*padPositionZ), glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    /** /
    cameraTransform = glm::rotate(float(sin(totalElapsedTime)), glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);
    /**/

    //glm::mat4 VP = projection * cameraTransform;

    // Move and rotate various SceneNodes
    boxNode->position = { 0, -10, -80 };

    ballNode->position = ballPosition;
    ballNode->scale = glm::vec3(ballRadius);
    ballNode->rotation = { 0, totalElapsedTime*2, 0 };

    padNode->position  = {
        boxNode->position.x - (boxDimensions.x/2) + (padDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x),
        boxNode->position.y - (boxDimensions.y/2) + (padDimensions.y/2),
        boxNode->position.z - (boxDimensions.z/2) + (padDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z)
    };

    updateNodeTransformations(rootNode, glm::identity<glm::mat4>());
    view = cameraTransform;
    glUniform3fv(shader->getUniformFromName("ball_pos"), 1, glm::value_ptr(glm::vec3(ballNode->currentTransformationMatrix*glm::vec4(0,0,0,1))));

}

void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar) {
    glm::mat4 transformationMatrix =
              glm::translate(node->position)
            * glm::translate(node->referencePoint)
            * glm::rotate(node->rotation.y, glm::vec3(0,1,0))
            * glm::rotate(node->rotation.x, glm::vec3(1,0,0))
            * glm::rotate(node->rotation.z, glm::vec3(0,0,1))
            * glm::scale(node->scale)
            * glm::translate(-node->referencePoint);

    node->currentTransformationMatrix = transformationThusFar * transformationMatrix; // M

    switch(node->nodeType) {
        case GEOMETRY: break;
        case SPOT_LIGHT: case POINT_LIGHT: 
            GLint pos_location = shader->getUniformFromName("light_info[" + std::to_string(node->lightID) + "].position");
            glUniform3fv(pos_location, 1, glm::value_ptr(glm::vec3(node->currentTransformationMatrix * glm::vec4(0,0,0,1))));

            GLint col_location = shader->getUniformFromName("light_info[" + std::to_string(node->lightID) + "].color");
            glUniform3fv(col_location, 1, glm::value_ptr(lightSources[node->lightID].lightColor));
            break;
        
    }

    for(SceneNode* child : node->children) {
        updateNodeTransformations(child, node->currentTransformationMatrix);
    }
}

void renderNode(SceneNode* node) {
    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix)); //M
    glUniformMatrix4fv(8, 1, GL_FALSE, glm::value_ptr(view)); //V
    glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(projection)); //P

    glm::mat3 normal_matrix = glm::mat3(glm::inverse(glm::transpose(node->currentTransformationMatrix)));
    glUniformMatrix3fv(5, 1, GL_FALSE, glm::value_ptr(normal_matrix));


    if (node->textureID != -1) {
        glUniform1i(6, 1); // do_texture
        glBindTextureUnit(0, node->textureID);
    } else  {
        glUniform1i(6, 0); // don't do_texture
    }

    if (node->roughnessMapID != -1) {
        glUniform1i(9, 1); // roughness
        glBindTextureUnit(2, node->roughnessMapID);
    } else  {
        glUniform1i(9, 0); // no roughness
    }

    if (node->isSkybox){
        glUniform1i(10, 1);
        glBindTextureUnit(3, node->textureID);
    }
    else{
        glUniform1i(10, 0);
    }

    switch(node->nodeType) {
        case GEOMETRY:
            if(node->vertexArrayObjectID != -1) {
                glUniform1i(7, 0); // is_3d
                if (!node->isSkybox){
                    glBindVertexArray(node->vertexArrayObjectID);
                    glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
                }
                else{
                    glDepthMask(GL_FALSE); //We want the skabox to be all the way in the back

                    //To do away with transelation
                    auto  view2 = glm::mat4(glm::mat3(view)); // store locally on stack
                    glUniformMatrix4fv(8, 1, GL_FALSE, glm::value_ptr(view2)); // V without translation
                    
                    glBindVertexArray(node->vertexArrayObjectID);
                    glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
                    glDepthMask(GL_TRUE);
                }
            }
            break;
        case GEOMETRY_2D:
            if(node->vertexArrayObjectID != -1) {
                glUniform1i(7, 1); // is_2d
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case GEOMETRY_NORMAL_MAPPED:
            if(node->vertexArrayObjectID != -1) {
                glUniform1i(7, 0); // is_3d
                glBindTextureUnit(1, node->normalMapTextureID);
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case POINT_LIGHT: break;
        case SPOT_LIGHT: break;
    }

    for(SceneNode* child : node->children) {
        renderNode(child);
    }
}

void renderFrame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    // set light uniforms, only applies to currently loaded shader
    /** /
    glm::vec3 lights[3];
    for (int i = 0; i < 3; i++) {
        lights[i] = glm::vec3(lightSources[i].lightNode->currentTransformationMatrix * glm::vec4(0,0,0,1)); //extract the position of each light
    }
    glUniform3fv(shader->getUniformFromName("lights"), 3, glm::value_ptr(lights[0]));
    /**/

   
    renderNode(rootNode);
}
