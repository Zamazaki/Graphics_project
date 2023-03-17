#pragma once

#include "mesh.h"
#include <vector>

unsigned int generateBuffer(Mesh &mesh);
void loadCubeMap(GLuint *unbound_int, std::vector<std::string> faces);
