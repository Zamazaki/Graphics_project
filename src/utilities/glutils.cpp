#include <glad/glad.h>
#include <program.hpp>
#include "glutils.h"
#include <vector>
#define OBJL_IMPLEMENTATION
#include "obj_loader.h"
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

unsigned int generateObjBuffer(objl_obj_file obj) {
    unsigned int vaoID;
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    std::vector<glm::vec3> vertices = {};
    std::vector<glm::vec3> vertex_normals = {};
    std::vector<glm::vec2> vertex_textureCoordiantes = {};
    std::vector<unsigned int> indices = {};

    for (int i = 0; i < obj.v_count; ++i) {
        vertices[i] = {obj.v[i].x, obj.v[i].y, obj.v[i].z};
    }
    generateAttribute(0, 3, vertices, false);

    if (obj.vn_count > 0) {
        for (int i = 0; i < obj.vn_count; ++i) {
            vertex_normals[i] = {obj.vn[i].x, obj.vn[i].y, obj.vn[i].z};
        }
        generateAttribute(1, 3, vertex_normals, true);
    }

    if (obj.vt_count > 0) {
        for (int i = 0; i < obj.vt_count; ++i) {
            vertex_textureCoordiantes[i] = {obj.vt[i].x, obj.vt[i].y};
        }
        generateAttribute(2, 2, vertex_textureCoordiantes, false);
    }

    if (obj.vn_count > 0 && obj.vt_count > 0){
        std::vector<glm::vec3> indexed_tangents;
        std::vector<glm::vec3> indexed_bitangents;

        computeTangentBasis(vertices, vertex_textureCoordiantes, vertex_normals, indexed_tangents, indexed_bitangents);

        generateAttribute(3, 3, indexed_tangents, true);

        generateAttribute(4, 3, indexed_bitangents, true);
        
    }
    
    for (int i = 0; i < obj.f_count; ++i) {
        indices[i*3] = obj.f[i].f0.vertex;
        indices[(i*3)+1] = obj.f[i].f1.vertex;
        indices[(i*3)+2] =  obj.f[i].f2.vertex;
    }

    unsigned int indexBufferID;
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.f_count* 3 * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    return vaoID;
}



unsigned int generateSkyboxBuffer(std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &indices) {
    unsigned int vaoID;
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    generateAttribute(0, 3, vertices, false);
    unsigned int indexBufferID;
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    return vaoID;
}

// I borrowed this from the opencl tutorial, if I need to reimplement it myself I can do that as well
unsigned int loadCubeMap(GLuint *unbound_int, std::vector<std::string> faces)
{
    //unsigned int textureID;
    glGenTextures(1, unbound_int);
    glBindTexture(GL_TEXTURE_CUBE_MAP, *unbound_int);

    int width, height, nrChannels;
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
            //std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    //return textureID;
} 
