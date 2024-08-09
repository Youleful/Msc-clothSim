/**
 * @file      main.cpp
 * @brief     Example N-body simulation for CIS 565
 * @authors   Liam Boone, Kai Ninomiya
 * @date      2013-2015
 * @copyright University of Pennsylvania
 */
#include "main.hpp"
#include "simulation.hpp"
#include <common/controls.hpp>

// ================
// Configuration
// ================

#define VISUALIZE 1

#define SPEED 1.0f
#define VEL 1.0f

const float DT = 0.3f;

Simulation *sim = NULL;

Camera camera;

void APIENTRY MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
}


//for the window resized
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);

	camera.projection = glm::perspective(fovy, float(width) / float(height), zNear, zFar);
	glm::mat4 view = glm::lookAt(camera.cameraPosition, glm::vec3(0), glm::vec3(0, 0, 1));

	camera.projection = camera.projection * view;
}

/**
 * C main function.
 */
int main(int argc, char* argv[]) {
    projectName = "Msc A GPU-Based streaming algorithm on cloth simulation";

    if (init(argc, argv)) {
        mainLoop();
        return 0;
    } else {
        return 1;
    }
}

//-------------------------------
//---------RUNTIME STUFF---------
//-------------------------------

std::string deviceName;
GLFWwindow *window;

/**
 * Initialization of GLFW.
 */
bool init(int argc, char **argv) {
    // Window setup stuff
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        std::cout
            << "Error: Could not initialize GLFW!"
            << " Perhaps OpenGL 3.3 isn't available?"
            << std::endl;
        return false;
    }
    
    // run some tests and unit tests

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, deviceName.c_str(), NULL, NULL);
    if (!window) {
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwPollEvents();
	glfwSetCursorPos(window, width / 2, height / 2);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        return false;
    }
    glGetError();

	/*glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);*/


	camera.cameraPosition.x = camera.zoom * sin(camera.phi) * sin(camera.theta);
	camera.cameraPosition.z = camera.zoom * cos(camera.theta);
	camera.cameraPosition.y = camera.zoom * cos(camera.phi) * sin(camera.theta);

	/*computeMatricesFromInputs();

	camera.cameraPosition = getCameraPosition();*/;
	camera.projection = glm::perspective(fovy, float(width) / float(height), zNear, zFar);
	glm::mat4 view = glm::lookAt(camera.cameraPosition, camera.lookAt, glm::vec3(0, 0, 1));
	camera.projection = camera.projection * view;

    initShaders(program);
	
	loadPerformanceTests();
	//loadDancingBear();
	//loadStaticCollDetectDebug();
	//loadStaticCollResolveDebug();

    glEnable(GL_DEPTH_TEST);

    return true;
}

void loadDancingBear() {
	std::vector<string> colliders;
	std::vector<string> cloths;
	// Initialize a fun simulation
	colliders.push_back("meshes/low_poly_bear.obj");
	colliders.push_back("meshes/floor.obj");

	//cloths.push_back("meshes/cape.obj");
	cloths.push_back("meshes/dress.obj");
	cloths.push_back("meshes/bear_cloth.obj");

	sim = new Simulation(colliders, cloths);

	// let's generate some clothespins!
	int bearLeftShoulder = 779;
	int bearRightShoulder = 1578;
	int bearSSBO = sim->rigids[0]->sPosBuffer;
	sim->rigids[0]->color = glm::vec3(1.0f);
	sim->rigids[0]->animated = true;

	// SSBOs for cape
	//int capeLeftShoulder = 1;
	//int capeRightShoulder = 0;
	//sim->cloths[0]->addPinConstraint(capeLeftShoulder, bearLeftShoulder, bearSSBO);
	//sim->cloths[0]->addPinConstraint(capeRightShoulder, bearRightShoulder, bearSSBO);
	//sim->cloths[0]->color = glm::vec3(0.5f, 1.0f, 1.0f);

	// SSBOs for dress
	int dressLeftShoulder = 14;
	int dressRightShoulder = 13;
	sim->cloths[0]->addPinConstraint(dressLeftShoulder, bearLeftShoulder, bearSSBO);
	sim->cloths[0]->addPinConstraint(dressRightShoulder, bearRightShoulder, bearSSBO);
	sim->cloths[0]->color = glm::vec3(1.0f, 0.5f, 0.5f);
}

void loadPerformanceTests() {
	std::vector<string> colliders;
	std::vector<string> cloths;
	//cloths.push_back("meshes/perf/cloth_256.obj");
	cloths.push_back("");
	colliders.push_back("meshes/perf/ball_98.obj");

	camera.zoom = 10.0f;
	updateCamera();
	sim = new Simulation(colliders, cloths);
}

void loadStaticCollDetectDebug() {
	std::vector<string> colliders;
	std::vector<string> cloths;
	cloths.push_back("meshes/perf/cloth_121.obj");
	colliders.push_back("meshes/perf/ball_98.obj");
	camera.zoom = 10.0f;
	updateCamera();
	camera.stepFrames = true;
	sim = new Simulation(colliders, cloths);
}

void loadStaticCollResolveDebug() {
	std::vector<string> colliders;
	std::vector<string> cloths;
	colliders.push_back("meshes/low_poly_bear.obj");
	colliders.push_back("meshes/floor.obj");

	//cloths.push_back("meshes/bear_cloth.obj");
	cloths.push_back("");
	sim = new Simulation(colliders, cloths);

	sim->rigids[0]->animated = false;
}

void initShaders(GLuint&  program) {
    GLint location;

	program = glslUtility::LoadGeomShaders(
		"shaders/cloth.vert",
		"shaders/cloth.geom",
		"shaders/cloth.frag", attributeLocations, 1);
	glUseProgram(program);

	if ((location = glGetUniformLocation(program, "u_projMatrix")) != -1) {
		glUniformMatrix4fv(location, 1, GL_FALSE, &camera.projection[0][0]);
	}

	updateCamera();
}

//====================================
// Main loop
//====================================

void mainLoop() {
    double fps = 0;
    double timebase = 0;
    int frame = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        frame++;
        double time = glfwGetTime();

        if (time - timebase > 1.0) {
            fps = frame / (time - timebase);
            timebase = time;
            frame = 0;
        }
        std::ostringstream ss;
        ss << "[";
        ss.precision(1);
        ss << std::fixed << fps;
        ss << " fps] " << time;
        glfwSetWindowTitle(window, ss.str().c_str());

		if (!camera.pause) {
			sim->stepSimulation();
			sim->updateGravity();
		}

#if VISUALIZE
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.5f, 0.7f, 1.0f, 1.0f);

		// draw all the meshes
		for (int i = 0; i < sim->numRigids; i++) {
			drawMesh(sim->rigids.at(i));
		}
		for (int i = 0; i < sim->numCloths; i++) {
			drawMesh(sim->cloths.at(i));
		}
		glfwSwapBuffers(window);
		if (camera.stepFrames)
			camera.pause = true;
#endif
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}

void drawMesh(Mesh *drawMe) {

  glUseProgram(program);
  glBindVertexArray(drawMe->drawingVAO);

  // upload color uniform
  GLint location;
  if ((location = glGetUniformLocation(program, "u_color")) != -1) {
	  glUniform3fv(location, 1, &drawMe->color[0]);
  }

  // Tell the GPU where the positions are. haven't figured out how to bind to the VAO yet
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, drawMe->sPosBuffer);

  // Draw the elements.
  //glDrawElements(GL_TRIANGLES, drawMe->indicesTris.size(), GL_UNSIGNED_INT, 0);
  if(drawMe->isCustomize)
	glDrawElements(GL_TRIANGLE_STRIP, drawMe->indices.size(), GL_UNSIGNED_INT, 0);
  else
	glDrawElements(GL_TRIANGLES, drawMe->indices.size(), GL_UNSIGNED_INT, 0);
  //checkGLError("visualize");
}

void errorCallback(int error, const char *description) {
    fprintf(stderr, "error %d: %s\n", error, description);
}



void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	static double lastTime = glfwGetTime();

	// Compute time difference between current and last frame
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);

	double startTime = 0;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
		return;
    }
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		camera.pause = !camera.pause;
		if (!camera.pause)
			startTime = currentTime;
		else
			cout << currentTime - startTime;
		return;
	}


	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera.lookAt.x += deltaTime * VEL;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera.lookAt.x -= deltaTime * VEL;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		camera.lookAt.y += deltaTime * VEL;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		camera.lookAt.y -= deltaTime * VEL;
	}

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		camera.lookAt.z -= deltaTime * VEL;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		camera.lookAt.z += deltaTime * VEL;
	}
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
		camera.zoom -= deltaTime * VEL;
	}
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
		camera.zoom += deltaTime * VEL;
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		camera.phi += deltaTime * SPEED;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		camera.phi -= deltaTime * SPEED;
	}
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		camera.theta -= deltaTime * SPEED;
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		camera.theta += deltaTime * SPEED;
	}

	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		camera.stepFrames = !camera.stepFrames;
	}
	lastTime = currentTime;
	updateCamera();
	//drawRaycast();
}

void updateCamera() {
	camera.cameraPosition.x = camera.zoom * sin(camera.phi) * sin(camera.theta);
	camera.cameraPosition.z = camera.zoom * cos(camera.theta);
	camera.cameraPosition.y = camera.zoom * cos(camera.phi) * sin(camera.theta);
	camera.cameraPosition += camera.lookAt;

	//computeMatricesFromInputs();
	//camera.cameraPosition = getCameraPosition();
	
	camera.projection = glm::perspective(fovy, float(width) / float(height), zNear, zFar);
	glm::mat4 view = glm::lookAt(camera.cameraPosition, camera.lookAt, glm::vec3(0, 0, 1));
	camera.projection = camera.projection * view;

	GLint location;

	glUseProgram(program);
	if ((location = glGetUniformLocation(program, "u_projMatrix")) != -1) {
		glUniformMatrix4fv(location, 1, GL_FALSE, &camera.projection[0][0]);
	}
	if ((location = glGetUniformLocation(program, "u_camLight")) != -1) {
		glUniform3fv(location, 1, &camera.cameraPosition[0]);
	}
}

