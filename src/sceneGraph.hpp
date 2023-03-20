#pragma once

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stack>
#include <vector>
#include <cstdio>
#include <stdbool.h>
#include <cstdlib> 
#include <ctime> 
#include <chrono>
#include <fstream>

enum SceneNodeType {
	GEOMETRY, POINT_LIGHT, SPOT_LIGHT, GEOMETRY_2D, GEOMETRY_NORMAL_MAPPED
};

struct SceneNode {
	SceneNode(SceneNodeType type) {
		position = glm::vec3(0, 0, 0);
		rotation = glm::vec3(0, 0, 0);
		scale = glm::vec3(1, 1, 1);

        referencePoint = glm::vec3(0, 0, 0);
        vertexArrayObjectID = -1;
        VAOIndexCount = 0;
		textureID = -1;
		lightID = -1;
		roughnessMapID = -1;
		metalRoughnessMapID = -1;
		isSkybox = false;

        nodeType = type;

	}

	// A list of all children that belong to this node.
	// For instance, in case of the scene graph of a human body shown in the assignment text, the "Upper Torso" node would contain the "Left Arm", "Right Arm", "Head" and "Lower Torso" nodes in its list of children.
	std::vector<SceneNode*> children;
	
	// The node's position and rotation relative to its parent
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	// A transformation matrix representing the transformation of the node's location relative to its parent. This matrix is updated every frame.
	glm::mat4 currentTransformationMatrix;

	// The location of the node's reference point
	glm::vec3 referencePoint;

	// The ID of the VAO containing the "appearance" of this SceneNode.
	int vertexArrayObjectID;
	unsigned int VAOIndexCount;

	// Node type is used to determine how to handle the contents of a node
	SceneNodeType nodeType;

	//Index in light struct if node is a light
	int lightID;

	//If node is texture then we must save its ID
	int textureID;

	// If node is normal texture we must have id for that as well
	int normalMapTextureID;

	// If node has roughness texture we must have id for that as well
	int roughnessMapID;

	// If node has metal and roughness maps combined in one map
	int metalRoughnessMapID;

	// If node is skybox, 1 for yes, 0 for no
	bool isSkybox;
};

SceneNode* createSceneNode(SceneNodeType type);
void addChild(SceneNode* parent, SceneNode* child);
void printNode(SceneNode* node);
int totalChildren(SceneNode* parent);

// For more details, see SceneGraph.cpp.