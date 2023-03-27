#pragma once

#include "mesh.h"
#include <vector>

unsigned int generateBuffer(Mesh &mesh);
void loadCubeMap(GLuint *unbound_int, std::vector<std::string> faces);
void initDynamicCube(GLuint *cubemap, GLuint *framebuffer, GLuint *depthbuffer);
void getDynamicCubeSides(GLuint framebuffer, int face, glm::mat4 *projection, glm::mat4 *view, glm::vec3 cameraPosition);
void endDynamicCubeMap();