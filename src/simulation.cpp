#include "simulation.hpp"
#include <random>

// for varying shader work group size.
// this gets injected into the shaders when they are loaded.
#define WORK_GROUP_SIZE_X 1024
#define WORK_GROUP_SIZE_Y 1

#define DEBUG_VERBOSE 0

#define QUERY_PERFORMANCE 1


int getWorkGroupX(int n)
{
	return (n  - 1) / WORK_GROUP_SIZE_X + 1;
}

int getWorkGroupY(int n)
{
	//return (n - 1) / WORK_GROUP_SIZE_Y + 1;
	return 1;
}

Simulation::Simulation(vector<string> &body_filenames,
	vector<string> &cloth_filenames) {
	initComputeProgs();
	glm::vec3 jitter;
	int iSecret;
	iSecret = rand() % 100 + 1;
	jitter.x = (float)iSecret / 100000.0f;
	iSecret = rand() % 100 + 1;
	jitter.y = (float)iSecret / 100000.0f;
	iSecret = rand() % 100 + 1;
	jitter.z = (float)iSecret / 100000.0f;

	numRigids = body_filenames.size();
	for (int i = 0; i < numRigids; i++) {
		Rbody *newCollider = new Rbody(body_filenames[i]);
		rigids.push_back(newCollider);
	}

	numCloths = cloth_filenames.size();
	for (int i = 0; i < numCloths; i++) {

		Cloth* newCloth;
		if (cloth_filenames[i] == "")
			newCloth = new Cloth(clothData);
		else
 			newCloth = new Cloth(cloth_filenames[i], jitter);
		
		
		iSecret = rand() % 100 + 1;
		jitter.x = (float)iSecret / 100000.0f;
		iSecret = rand() % 100 + 1;
		jitter.y = (float)iSecret / 100000.0f;
		iSecret = rand() % 100 + 1;
		jitter.z = (float)iSecret / 100000.0f;
		newCloth->uploadExternalConstraints();
		cloths.push_back(newCloth);
	}

#if QUERY_PERFORMANCE
	glGenQueries(1, &time_query);
	elapsed_time = 0;
	frameCount = 0;

	for (int i = 0; i < 7; ++i)
	{
		timeStagesTotal[i] = 0.0f;
		timeStagesAVG[i] = 0.0f;
		timeStagesMin[i] = HUGE_VAL;
		timeStagesMax[i] = 0;
	}
#endif

}

Simulation::~Simulation() {
	// delete all the meshes and rigidbodies
	for (int i = 0; i < numRigids; i++) {
		delete(&rigids.at(i));
	}
	for (int i = 0; i < numCloths; i++) {
		delete(&cloths.at(i));
	}
}

//http://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

GLuint initComputeProg(const char *path) {

	return glslUtility::LoadCompShaders(path);
}

void Simulation::updateStat(int stat) {
	glEndQuery(GL_TIME_ELAPSED);

	// wait until the query is available and return.
	int done = 0;
	while (!done) {
		glGetQueryObjectiv(time_query,
			GL_QUERY_RESULT_AVAILABLE,
			&done);
	}
	glGetQueryObjectui64v(time_query, GL_QUERY_RESULT, &elapsed_time);
	timeStagesTotal[stat] += elapsed_time;
	timeStagesAVG[stat] = (float)timeStagesTotal[stat] / (float)frameCount;

	timeStagesMin[stat] = glm::min(timeStagesMin[stat], elapsed_time);
	timeStagesMax[stat] = glm::max(timeStagesMax[stat], elapsed_time);
}

void Simulation::updateGravity()
{

	if (simAttri.isWind) {
			std::default_random_engine rndEngine((unsigned)time(nullptr));
			std::uniform_real_distribution<float> rd(1.0f, 2.0f);
			simAttri.Gravity.x = cos(glm::radians(-simAttri.timeStep * 360.0f)) * (rd(rndEngine) - rd(rndEngine));
			simAttri.Gravity.y = sin(glm::radians(simAttri.timeStep * 360.0f)) * (rd(rndEngine) - rd(rndEngine));
	}

	glUseProgram(compExternalForces);
	glUniform3fv(1, 1, &simAttri.Gravity[0]);
	glUniform1f(0, simAttri.timeStep);
}

void Simulation::initComputeProgs() {
	compExternalForces = initComputeProg("shaders/cloth_pbd1_externalForces.comp");	
	glUseProgram(compExternalForces);
	glUniform3fv(1, 1, &simAttri.Gravity[0]);
	glUniform1f(0, simAttri.timeStep);

	//prog_ppd2_dampVelocity = initComputeProg("shaders/cloth_pbd2_dampVelocities.comp");

	compPredPositions = initComputeProg("shaders/cloth_pbd3_predictPositions.comp");
	glUseProgram(compPredPositions);
	glUniform1f(0, simAttri.timeStep);

	compUpdateInvMass = initComputeProg("shaders/cloth_pbd4_updateInverseMasses.comp");

	compProjectClothConstraints = initComputeProg("shaders/cloth_pbd5_projectClothConstraints.comp");
	glUseProgram(compProjectClothConstraints);
	glUniform1f(0, (float)simAttri.projectTimes);

	compUpdateVelPos = initComputeProg("shaders/cloth_pbd6_updatePositionsVelocities.comp");
	glUseProgram(compUpdateVelPos);
	glUniform1f(0, simAttri.timeStep);

	
	compCopyBuffer = initComputeProg("shaders/copy.comp");

	compGenCollisionConstraints = initComputeProg("shaders/cloth_genCollisions.comp");

	compProjectCollisionConstraints = initComputeProg("shaders/cloth_projectCollisions.comp");
	glUseProgram(compProjectCollisionConstraints);
	glUniform1f(1, simAttri.collisionBounceFactor);

	compRigidbodyAnimate = initComputeProg("shaders/rigidbody_animate.comp");
}

void Simulation::genCollisionConstraints(Cloth *cloth, Rbody *rbody) {
	int numVertices = cloth->initPositions.size();
	glUseProgram(compGenCollisionConstraints);
	glUniform1i(0, rbody->indices.size() / 3);
	glUniform1i(1, numVertices);
	glUniform1f(2, cloth->clothAttri.static_constraint_bounce);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->sPosBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->sCurPredPosBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, rbody->sPosBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, rbody->sTriangles);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, cloth->sCollisionConstraints);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, cloth->sDebugBuffer);

	int workGroupCount_vertices_x = getWorkGroupX(numVertices);           
	int workGroupCount_vertices_y = getWorkGroupY(numVertices);

	glDispatchCompute(workGroupCount_vertices_x, workGroupCount_vertices_y, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//retrieveBuffer(cloth->ssbo_debug, 121);
}

void Simulation::stepSingleCloth(Cloth *cloth) {
	int numVertices = cloth->initPositions.size();
	int workGroupCount_vertices_x = getWorkGroupX(numVertices);
	int workGroupCount_vertices_y = getWorkGroupY(numVertices);


	/* compute new velocities with external forces */

#if QUERY_PERFORMANCE
	glBeginQuery(GL_TIME_ELAPSED, time_query);
#endif

	glUseProgram(compExternalForces);
	glUniform1i(2, numVertices);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->sVelBuffer);
	glDispatchCompute(workGroupCount_vertices_x , workGroupCount_vertices_y, 1);
	//prevent bug from the parallization
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); 
#if QUERY_PERFORMANCE
	updateStat(EXTERNAL_FORCE);
#endif

	/* damp velocities */
	/*glUseProgram(prog_ppd2_dampVelocity);
	glUniform1i(0, numVertices);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->sVelBuffer);
	glDispatchCompute(workGroupCount_vertices, 1, 1);*/
	
#if QUERY_PERFORMANCE
	glBeginQuery(GL_TIME_ELAPSED, time_query);
#endif
	/* predict new positions */
	glUseProgram(compPredPositions);
	glUniform1i(1, numVertices);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->sVelBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->sPosBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->sPrevPredPosBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cloth->sCurPredPosBuffer);
	glDispatchCompute(workGroupCount_vertices_x, workGroupCount_vertices_y, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

#if QUERY_PERFORMANCE
	updateStat(PRED_POSITION);
#endif

#if QUERY_PERFORMANCE
	glBeginQuery(GL_TIME_ELAPSED, time_query);
#endif
	/* update inverse masses */
	int numPinConstraints = cloth->externalConstraints.size();
	int workGroupCountPinConstraints_X= getWorkGroupX(numPinConstraints); 
	int workGroupCountPinConstraints_Y = getWorkGroupY(numPinConstraints);
	glUseProgram(compUpdateInvMass);
	glUniform1i(0, numPinConstraints);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->sPrevPredPosBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->sCurPredPosBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->sExternalConstraints);
	glDispatchCompute(workGroupCountPinConstraints_X, workGroupCountPinConstraints_Y, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

#if QUERY_PERFORMANCE
	updateStat(UPDATE_INVMASS);
#endif


#if QUERY_PERFORMANCE
	glBeginQuery(GL_TIME_ELAPSED, time_query);
#endif
	/* project cloth constraints N times */
	for (int i = 0; i < simAttri.projectTimes; i++) {
		glUseProgram(compProjectClothConstraints);
		// project each of the 4 internal constraints
		// bind predicted positions input/output
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->sPrevPredPosBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->sCurPredPosBuffer);
		glUniform1f(2, cloth->clothAttri.internal_K); // uniform K
		glUniform1i(3, cloth->sPrevPredPosBuffer); // send the identity of the influencing SSBO

		for (int j = 0; j < cloth->numInternalConstraintBuffers; j++) {
			// bind inner constraints
			int workGroupCountInnerConstraints_x = getWorkGroupX(cloth->internalConstraints[j].size());
			int workGroupCountInnerConstraints_y = getWorkGroupY(cloth->internalConstraints[j].size());
			glUniform1i(1, cloth->internalConstraints[j].size());

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->sInternalConstraints[j]);
			// project this set of constraints
			glDispatchCompute(workGroupCountInnerConstraints_x, workGroupCountInnerConstraints_y, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		}

		// project pin constraints
		int numPinnedSSBOs = cloth->pinnedSSBOs.size();
		for (int i = 0; i < numPinnedSSBOs; i++) {
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->pinnedSSBOs[i]); // init positions, not pred 1
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->sCurPredPosBuffer); // update this
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->sExternalConstraints);
			glUniform1i(1, cloth->externalConstraints.size());
			glUniform1f(2, cloth->clothAttri.pin_K); // uniform K
			glUniform1i(3, (int)cloth->pinnedSSBOs[i]); // send the identity of the influencing SSBO
			glDispatchCompute(workGroupCountPinConstraints_X, workGroupCountPinConstraints_Y, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		// ffwd pred1 to match pred2
		glUseProgram(compCopyBuffer); // TODO: lol... THIS IS DUMB DO SOMETHING BETTER
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->sCurPredPosBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->sPrevPredPosBuffer);
		glDispatchCompute(workGroupCount_vertices_x, workGroupCount_vertices_y, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

#if QUERY_PERFORMANCE
	updateStat(PROJ_CONSTRAINTS);
#endif

#if QUERY_PERFORMANCE
	glBeginQuery(GL_TIME_ELAPSED, time_query);
#endif

	

	/* generate and resolve collision constraints */
	for (int i = 0; i < numRigids; i++) {
		genCollisionConstraints(cloth, rigids.at(i));
	}

#if QUERY_PERFORMANCE
	updateStat(GENER_COLLISIONS);
#endif

#if DEBUG_VERBOSE
	cout << "init ";
	retrieveBuffer(cloth->sPosBuffer, 1);
	cout << "pred befor ";
	retrieveBuffer(cloth->sCurPredPosBuffer, 1);
#endif

#if QUERY_PERFORMANCE
	glBeginQuery(GL_TIME_ELAPSED, time_query);
#endif

	glUseProgram(compProjectCollisionConstraints);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->sPosBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->sCurPredPosBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->sCollisionConstraints);
	glUniform1i(0, numVertices);
	glDispatchCompute(workGroupCount_vertices_x, workGroupCount_vertices_y, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

#if QUERY_PERFORMANCE
	updateStat(RESOL_COLLISIONS);
#endif

#if DEBUG_VERBOSE
	cout << "pred after ";
	retrieveBuffer(cloth->sCurPredPosBuffer, 1);
	cout << "con ";
	retrieveBuffer(cloth->sCollisionConstraints, 1);
#endif
	//retrieveBuffer(cloth->sCollisionConstraints, 1);
	/* update positions and velocities, reset collision constraints */

#if QUERY_PERFORMANCE
	glBeginQuery(GL_TIME_ELAPSED, time_query);
#endif
	glUseProgram(compUpdateVelPos);
	glUniform1i(1, numVertices);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->sVelBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->sPosBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->sCurPredPosBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cloth->sCollisionConstraints);
	glDispatchCompute(workGroupCount_vertices_x, workGroupCount_vertices_y, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

#if QUERY_PERFORMANCE
	updateStat(UPDATE_POSVEL);
#endif

#if DEBUG_VERBOSE
	cout << "vel ";
	retrieveBuffer(cloth->sVelBuffer, 1);
	cout << endl;
#endif
}

void Simulation::retrieveBuffer(GLuint ssbo, int numItems) {
	// test getting something back from the GPU. can we even do this?!
	GLint bufMask = GL_MAP_READ_BIT;
	
	std::vector<glm::vec4> positions;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glm::vec4 *constraintsMapped = (glm::vec4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, numItems * sizeof(glm::vec4), bufMask);
	for (int i = 0; i < numItems; i++) {
		positions.push_back(glm::vec4(constraintsMapped[i]));
	}
 	for (int i = 0; i < numItems; i++) {
		if (positions.at(i).w > 0.0001f && ((int)positions.at(i).w % 2 || positions.at(i).w > 2.0f))
			cout << positions.at(i).x << " " << positions.at(i).y << " " << positions.at(i).z << " " << positions.at(i).w << endl;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void Simulation::animateRbody(Rbody *rbody) {
	if (rbody->animated == false) return;
	glm::mat4 tf = rbody->getTransformationAtTime(simAttri.currentTime);
	int numVertices = rbody->initPositions.size();
	int workGroupCount_vertices_x = getWorkGroupX(numVertices);
	int workGroupCount_vertices_y = getWorkGroupY(numVertices);
	glUseProgram(compRigidbodyAnimate);
	glUniform1i(0, numVertices);
	glUniformMatrix4fv(1, 1, GL_FALSE, &tf[0][0]);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, rbody->sInitPos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rbody->sPosBuffer);
	glDispatchCompute(workGroupCount_vertices_x, workGroupCount_vertices_y, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Simulation::stepSimulation() {
	frameCount++;

	for (int i = 0; i < numCloths; i++) {
		stepSingleCloth(cloths.at(i));
	}

	for (int i = 0; i < numRigids; i++) {
		animateRbody(rigids[i]);
	}
	simAttri.currentTime += simAttri.timeStep;

#if QUERY_PERFORMANCE
	// report performance every n frames
	if (frameCount % 1000 == 0) {
		cout << "performance as of frame " << frameCount << endl;

		cout << "Iteration time:" << simAttri.projectTimes <<endl;
		cout << "Time step:" << simAttri.timeStep <<endl;
		cout << "average run time (ms/frame)" << endl;
		cout << "Predict position and velocity    " <<
			(float)timeStagesAVG[EXTERNAL_FORCE] / 1000.0f + (float)timeStagesAVG[PRED_POSITION] / 1000.0f + (float)timeStagesAVG[PRED_POSITION] / 1000.0f << endl;
		cout << "Project iteration:     " <<
			(float)timeStagesAVG[PROJ_CONSTRAINTS] / 1000.0f << endl;
		cout << "generating collision constraints: " <<
			(float)timeStagesAVG[GENER_COLLISIONS] / 1000.0f << endl;
		cout << "Project collision constraints:  " <<
			(float)timeStagesAVG[RESOL_COLLISIONS] / 1000.0f << endl;
		cout << "update position and velocity:  " <<
			(float)timeStagesAVG[UPDATE_POSVEL] / 1000.0f << endl;


		cout << "max times (microseconds)" << endl;
		cout << "calculate externol force:     " <<
			(float)timeStagesMax[EXTERNAL_FORCE] / 1000.0f << endl;
		cout << "predict position: " <<
			(float)timeStagesMax[PRED_POSITION] / 1000.0f << endl;
		cout << "update the inverse Mass:  " <<
			(float)timeStagesMax[UPDATE_INVMASS] / 1000.0f << endl;
		cout << "solving internal constraints:     " <<
			(float)timeStagesMax[PROJ_CONSTRAINTS] / 1000.0f << endl;
		cout << "generating collision constraints: " <<
			(float)timeStagesMax[GENER_COLLISIONS] / 1000.0f << endl;
		cout << "resolving collision constraints:  " <<
			(float)timeStagesMax[RESOL_COLLISIONS] / 1000.0f << endl;
		cout << "update position and velocity:  " <<
			(float)timeStagesMax[UPDATE_POSVEL] / 1000.0f << endl;
	}
#endif

}
