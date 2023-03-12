#pragma once

#include "mesh.h"
#include <vector>

unsigned int generateBuffer(Mesh &mesh);
unsigned int loadCubeMap(GLuint *unbound_int, std::vector<std::string> faces);
unsigned int generateSkyboxBuffer(std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &indices);