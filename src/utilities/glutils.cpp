#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <program.hpp>
#include "glutils.h"
#include <vector>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


template <class T>
unsigned int generateAttribute(int id, int elementsPerEntry, std::vector<T> data, bool normalize) {
    unsigned int bufferID;
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(id, elementsPerEntry, GL_FLOAT, normalize ? GL_TRUE : GL_FALSE, sizeof(T), 0);
    glEnableVertexAttribArray(id);
    return bufferID;
}

void computeTangentBasis( // I stole this from the tutorial
    // inputs
    std::vector<glm::vec3> & vertices,
    std::vector<glm::vec2> & uvs,
    std::vector<glm::vec3> & normals,
    // outputs
    std::vector<glm::vec3> & tangents,
    std::vector<glm::vec3> & bitangents
){
    for ( int i=0; i<vertices.size(); i+=3){

        // Shortcuts for vertices
        glm::vec3 & v0 = vertices[i+0];
        glm::vec3 & v1 = vertices[i+1];
        glm::vec3 & v2 = vertices[i+2];

        // Shortcuts for UVs
        glm::vec2 & uv0 = uvs[i+0];
        glm::vec2 & uv1 = uvs[i+1];
        glm::vec2 & uv2 = uvs[i+2];

        // Edges of the triangle : position delta
        glm::vec3 deltaPos1 = v1-v0;
        glm::vec3 deltaPos2 = v2-v0;

        // UV delta
        glm::vec2 deltaUV1 = uv1-uv0;
        glm::vec2 deltaUV2 = uv2-uv0;

        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        glm::vec3 tangent = (deltaPos1 * deltaUV2.y   - deltaPos2 * deltaUV1.y)*r;
        glm::vec3 bitangent = (deltaPos2 * deltaUV1.x   - deltaPos1 * deltaUV2.x)*r;

        // Set the same tangent for all three vertices of the triangle.
        // They will be merged later
        tangents.push_back(tangent);
        tangents.push_back(tangent);
        tangents.push_back(tangent);

        // Same thing for bitangents
        bitangents.push_back(bitangent);
        bitangents.push_back(bitangent);
        bitangents.push_back(bitangent);
    }
}


unsigned int generateBuffer(Mesh &mesh) {
    unsigned int vaoID;
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    generateAttribute(0, 3, mesh.vertices, false);
    if (mesh.normals.size() > 0) {
        generateAttribute(1, 3, mesh.normals, true);
    }
    if (mesh.textureCoordinates.size() > 0) {
        generateAttribute(2, 2, mesh.textureCoordinates, false);
    }

    if (mesh.normals.size() > 0 && mesh.textureCoordinates.size() > 0){
        std::vector<glm::vec3> indexed_tangents;
        std::vector<glm::vec3> indexed_bitangents;

        computeTangentBasis(mesh.vertices, mesh.textureCoordinates, mesh.normals, indexed_tangents, indexed_bitangents);

        generateAttribute(3, 3, indexed_tangents, true);

        generateAttribute(4, 3, indexed_bitangents, true);
        
    }
    
    unsigned int indexBufferID;
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);

    return vaoID;
}

// I borrowed this from the opencl tutorial, if I need to reimplement it myself I can do that as well
void loadCubeMap(GLuint *unbound_int, std::vector<std::string> faces){
    glGenTextures(1, unbound_int);
    glBindTexture(GL_TEXTURE_CUBE_MAP, *unbound_int);

    int width, height, nrChannels; // (Cubemap width and height is 2048)
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);  

}


void initDynamicCube(GLuint *cubemap, GLuint *framebuffer, GLuint *depthbuffer){
    // create the fbo
    glGenFramebuffers(1, framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);

    // create the cubemap
    glGenTextures(1, cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, *cubemap);
    

    // set textures
    for (int i = 0; i < 6; ++i){
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 2048, 2048, 0, GL_RGB, GL_UNSIGNED_BYTE, 0); //Same heigh and with as regular cubemap
    }

    // Texture settings
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
    // create the uniform depth buffer
    glGenRenderbuffers(1, depthbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, *depthbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 2048, 2048);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
        
    // attach it
    //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, *framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, *depthbuffer);
    // attach only the +X cubemap texture (for completeness)
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, *cubemap, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
}


void getDynamicCubeSides(GLuint framebuffer, int face, glm::mat4 *projection, glm::mat4 *view, glm::vec3 cameraPosition){
    //Change viewport when drawing
    //glViewport(0, 0, 2048, 2048); 

    // attach new texture and renderbuffer to fbo 
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, cubemap, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, (GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face), framebuffer, 0);
        
    // clear
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //glMatrixMode(GL_PROJECTION);
    //glLoadIdentity();
  

    *projection =  glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 350.f); //Change aspect ratio into nice squares

    //glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();
        

    // setup lookat depending on current face
    switch (GL_TEXTURE_CUBE_MAP_POSITIVE_X + face)
    {
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X: //POSITIVE_X  
            *view = glm::lookAt( glm::vec3(0.0, 0.0, 0.0), glm::vec3(10.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)); 
            break;
                
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:  //NEGATIVE_X
            *view = glm::lookAt( glm::vec3(0.0, 0.0, 0.0), glm::vec3(-10.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
            break;
                
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:  //POSITIVE_Y
            *view = glm::lookAt( glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 10.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
            break;
                
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:  //NEGATIVE_Y
            *view = glm::lookAt( glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, -10.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
            break;
                
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:  //POSITIVE_Z
            *view = glm::lookAt( glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 10.0), glm::vec3(0.0, -1.0, 0.0));
            break;
                
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:  //NEGATIVE_Z
            *view = glm::lookAt( glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, -10.0), glm::vec3(0.0, -1.0, 0.0));
            break;
                
        default:
            break;
    };

    //*view = glm::translate(-cameraPosition) * (*view); //glTranslatef(-renderPosition.x, -renderPosition.y, -renderPosition.z);
    *view =  (*view) * glm::translate(-cameraPosition); //glTranslatef(-renderPosition.x, -renderPosition.y, -renderPosition.z);
    //*view = glm::translate(-glm::vec3(10, -20, -40)) * (*view);
    
}

void endDynamicCubeMap(){
    // disable
    glBindFramebuffer(GL_FRAMEBUFFER, 0);    
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}