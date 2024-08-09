#pragma once

#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <fstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <common/utils.hpp>
#include "mesh.hpp"

//====================================
// GL Stuff
//====================================

#define PI                          3.1415926535897932384626422832795028841971


GLuint attr_position = 0;
const char *attributeLocations[] = { "Position" };
GLuint displayImage;
GLuint program;

int width = 1280;
int height = 720;

const float fovy = (float) (PI / 4);
const float zNear = 0.10f;
const float zFar = 100.0f;

struct Camera
{
	glm::mat4 projection;
	float theta = 1.22f;
	float phi = -0.65f;
	float zoom = 5.0f;
	glm::vec3 lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
	bool pause = true;
	bool stepFrames = false;

	glm::vec3 cameraPosition = glm::vec3(0.0f);
};
//====================================
// Main
//====================================

const char *projectName;

int main(int argc, char* argv[]);

//====================================
// Main loop
//====================================
void mainLoop();
void drawMesh(Mesh *drawMe);
void errorCallback(int error, const char *description);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void updateCamera();

//====================================
// Setup/init Stuff
//====================================
bool init(int argc, char **argv);
void initShaders(GLuint& program);

//====================================
// room for different simualtions and
// gl unit tests
//====================================

void loadPerformanceTests();
void loadDancingBear(); // the default sim
void loadStaticCollDetectDebug(); // ball and cloth. debugging.
void loadStaticCollResolveDebug(); // floor and cloth. debugging.