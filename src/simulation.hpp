#pragma once
#include <vector>
#include "mesh.hpp"
#include "cloth.hpp"
#include "rbody.hpp"
#include <common/utils.hpp>

using namespace std;


enum State
{
	EXTERNAL_FORCE = 0,
	PRED_POSITION,
	UPDATE_INVMASS,
	UPDATE_POSVEL,
	PROJ_CONSTRAINTS,
	GENER_COLLISIONS,
	RESOL_COLLISIONS
};


struct SimulationAttri
{
	int projectTimes = 100;
	float timeStep = 0.016f;
	float currentTime = 0.0f;
	glm::vec3 Gravity = glm::vec3(0.0f, 0.0f, -0.98f);

	bool isWind = false;

	float collisionBounceFactor = 0.1f;
};

class Simulation
{
private:
	GLuint time_query;
	GLuint64 elapsed_time;
	GLuint64 frameCount;
	GLuint64 timeStagesTotal[7]; // ns
	float timeStagesAVG[7]; // ns
	GLuint64 timeStagesMin[7]; // ns
	GLuint64 timeStagesMax[7]; // ns

	void updateStat(int stat);


public:
	Simulation(vector<string> &body_filenames, vector<string> &cloth_filenames);
	~Simulation();

	vector<Rbody*> rigids;
	vector<Cloth*> cloths;

	ClothData clothData;
	int numRigids;
	int numCloths;

	GLuint compExternalForces;
	GLuint compPredPositions;
	GLuint compUpdateInvMass; // updates inverse masses for pinned vertices

	GLuint compProjectClothConstraints;
	GLuint compUpdateVelPos;
	GLuint compCopyBuffer;
	
	GLuint compGenCollisionConstraints;
	GLuint compProjectCollisionConstraints;

	GLuint compRigidbodyAnimate;

	SimulationAttri simAttri;

	void animateRbody(Rbody* rbody);

	void retrieveBuffer(GLuint ssbo, int numItems);

	void initComputeProgs();
	void genCollisionConstraints(Cloth *cloth, Rbody *rbody);
	void stepSingleCloth(Cloth *cloth);
	void stepSimulation();

	void updateGravity();
};