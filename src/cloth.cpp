#include "cloth.hpp"


Cloth::Cloth(string filename, glm::vec3 jitter) : Mesh(filename, jitter) {
	
	initialClothBuffer();
	color = glm::vec3(0.0f, 0.5f, 1.0f);
}

Cloth::Cloth(const ClothData& clothData) : Mesh(clothData)
{

	this->isCustomize = true;
	initialClothBuffer();
	color = glm::vec3(0.0f, 0.5f, 1.0f);
}

Cloth::~Cloth() {

}

struct constraintVertexIndices {
  int thisIndex = -1;
  int numConstraints = 0;
  int indices[NUM_INT_CON_BUFFERS];
};

void addConstraint(constraintVertexIndices &vert1, constraintVertexIndices &vert2) {
  // assess whether the constraints given already exist here or not
	for (int i = 0; i < NUM_INT_CON_BUFFERS; i++) {
    if (vert1.indices[i] == vert2.thisIndex) {
      return;
    }
    if (vert2.indices[i] == vert1.thisIndex) {
      return;
    }
  }

  // set up constraints on vert and influencer based on each other
	if (vert1.numConstraints < NUM_INT_CON_BUFFERS) {
    vert1.indices[vert1.numConstraints] = vert2.thisIndex;
    vert1.numConstraints++;
  }
	if (vert2.numConstraints < NUM_INT_CON_BUFFERS) {
    vert2.indices[vert2.numConstraints] = vert1.thisIndex;
    vert2.numConstraints++;
  }
}

void Cloth::generateConstraints() {

  /*****************************************************************************
   The standard mass-spring system constraint requires:
   - (1) index of vertex being moved
   - (1) index of vertex "not being moved"
   - (1) rest length
   So each constraint "struct" is unidirectional; real constraints are thus
   made up of 2 constraints each.
   The obj is loaded such that each face is "half-edged," counterclockwise.
   1---2---5
   |  \|/  |        N
   4---3---6  ->  W 3 E
   |  /|\  |        S
   44--43--45
   So we need to generate a constraint for all edges specified by indices
   In addition, we need to generate constraints for all indices that don't have
   a matched pair (in this case, 1-2, 2-5, etc.)

   Finally, since each constraint is designed to correct the predicted position
   of the "receiving" vertex and each vertex as such will have at most 4
   constraints (we're ignoring bending for now b/c we're super bad) we must
   use 4 buffers of constraints to avoid race conditions.

   So what we'll do is determine for each vertex just what constraints it should
   have, then distribute each vert's constraints into each of the 4 buffers.
   We'll do this by building 2 constraints for every edge given in a quadface.
   We don't actually NEED the N,S,E,W constraints to be uniform like this if
   we have a predicted position lag (one buffer stays "one projection behind"
   and has to be ffwded on the next step)
  *****************************************************************************/
  int numVertices = initPositions.size();
  std::vector<constraintVertexIndices> constraintsPerVertex;
  for (int i = 0; i < numVertices; i++) {
    constraintVertexIndices emptyVert;
    emptyVert.thisIndex = i;
	for (int j = 0; j < NUM_INT_CON_BUFFERS; j++) {
		emptyVert.indices[j] = -1;
	}
    constraintsPerVertex.push_back(emptyVert);
  }
  // Check every face and input the constraints per vertex.
  int numFaces = QuadsIndices.size();
  for (int i = 0; i < numFaces; i++) {    // each face generates 4 constraints
    glm::ivec4 face = QuadsIndices[i];
    addConstraint(constraintsPerVertex[face[0]], constraintsPerVertex[face[1]]);
    addConstraint(constraintsPerVertex[face[1]], constraintsPerVertex[face[2]]);
    addConstraint(constraintsPerVertex[face[2]], constraintsPerVertex[face[3]]);
    addConstraint(constraintsPerVertex[face[3]], constraintsPerVertex[face[0]]);
	if (NUM_INT_CON_BUFFERS > 4) {
		// diagonal constraints
		addConstraint(constraintsPerVertex[face[0]], constraintsPerVertex[face[2]]);
		addConstraint(constraintsPerVertex[face[1]], constraintsPerVertex[face[3]]);
	}
  }

  // deploy internal constraints and buffers

  for (int i = 0; i < numVertices; i++) {
    constraintVertexIndices currSet = constraintsPerVertex[i];
	for (int j = 0; j < NUM_INT_CON_BUFFERS; j++) {
      if (currSet.indices[j] != -1) {
        glm::vec4 shaderConstraint;
        shaderConstraint[0] = currSet.thisIndex;
        shaderConstraint[1] = currSet.indices[j];
		glm::vec4 p1 = initPositions[currSet.thisIndex];
		glm::vec4 p2 = initPositions[currSet.indices[j]];
		shaderConstraint[2] = glm::length(glm::vec3(p1) - 
			glm::vec3(p2));
		internalConstraints[j].push_back(shaderConstraint);
      }
    }
  }

  GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

  for (int i = 0; i < NUM_INT_CON_BUFFERS; i++) {
    // gen buffer
	glGenBuffers(1, &sInternalConstraints[i]);

    // allocate space for internal constraints on GPU
	int numConstraints = internalConstraints[i].size();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sInternalConstraints[i]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numConstraints * sizeof(glm::vec4),
      NULL, GL_STREAM_COPY);

    // transfer
    glm::vec4 *constraintsMapped = (glm::vec4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
      0, numConstraints * sizeof(glm::vec4), bufMask);

    for (int j = 0; j < numConstraints; j++) {
		// all internal constraints use ssbo_pos_pred1 as their influencer
		internalConstraints[i][j].w = (float) sPrevPredPosBuffer;
		constraintsMapped[j] = internalConstraints[i][j];
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  }

  /*****************************************************************************
  Set up the pins. These are the same format, but a negative rest length will
  signal to the shader that "hey this is a pin constraint."
  This is necessary for the step in which we update the inverse masses.
  *****************************************************************************/

  // make bufer for the external constraints (pins)
  glGenBuffers(1, &sExternalConstraints);
  // these are constraints for bear_cloth to pin to its initial position
  //addPinConstraint(0, 0, ssbo_pos);
  //addPinConstraint(40, 40, ssbo_pos);

  // transfer
  //uploadExternalConstraints();

  /*****************************************************************************
   Collision constraints need to be handled a little differently and will have
   a different format. Either way, the most possible is 1 per vertex.
  *****************************************************************************/

  // make space for collision constraints. these are per-vertex
  glGenBuffers(1, &sCollisionConstraints);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, sCollisionConstraints);
  glBufferData(GL_SHADER_STORAGE_BUFFER, numVertices * sizeof(glm::vec4),
	  NULL, GL_STREAM_COPY);
  // transfer bogus
  // collision constraints are (position vec3, bogusness)
  glm::vec4 *constraintsMapped = (glm::vec4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
	  0, numVertices * sizeof(glm::vec4), bufMask);
  glm::vec4 bogus = glm::vec4(-1.0f);
  for (int j = 0; j < numVertices; j++) {
	  constraintsMapped[j] = bogus;
  }
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void Cloth::initialClothBuffer()
{

	GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	int positionCount = initPositions.size();

	glGenBuffers(1, &sVelBuffer);
	glGenBuffers(1, &sPrevPredPosBuffer);
	glGenBuffers(1, &sCurPredPosBuffer);
	glGenBuffers(1, &sDebugBuffer);

	// redo the positions buffer with masses

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sPosBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, positionCount * sizeof(glm::vec4),
		NULL, GL_STREAM_COPY);
	glm::vec4* pos = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, positionCount * sizeof(glm::vec4), bufMask);
	for (int i = 0; i < positionCount; i++) {
		initPositions[i].w = clothAttri.inv_mass;
		pos[i] = initPositions[i];
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	// set up ssbo for velocities
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sVelBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, positionCount * sizeof(glm::vec4),
		NULL, GL_STREAM_COPY);
	glm::vec4* velocity = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, positionCount * sizeof(glm::vec4), bufMask);
	for (int i = 0; i < positionCount; i++) {
		velocity[i] = glm::vec4(0.0f);
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	// set up ssbo for predicted positions
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sPrevPredPosBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, positionCount * sizeof(glm::vec4),
		NULL, GL_STREAM_COPY);
	glm::vec4* ppos1 = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, positionCount * sizeof(glm::vec4), bufMask);
	for (int i = 0; i < positionCount; i++) {
		ppos1[i] = initPositions[i];
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	// set up another ssbo for predicted positions. ping-pong
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sCurPredPosBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, positionCount * sizeof(glm::vec4),
		NULL, GL_STREAM_COPY);
	glm::vec4* ppos2 = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, positionCount * sizeof(glm::vec4), bufMask);
	for (int i = 0; i < positionCount; i++) {
		ppos2[i] = initPositions[i];
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sDebugBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, positionCount * sizeof(glm::vec4),
		NULL, GL_STREAM_COPY);

	// set up constraints
	generateConstraints();
}

void Cloth::addPinConstraint(int thisIdx, int otherIdx, GLuint SSBO_ID) {
	externalConstraints.push_back(glm::vec4(thisIdx, otherIdx, -1.0, (int)SSBO_ID));
	uploadExternalConstraints();
	// search the current SSBO list to see if we need to add this one
	int numPinnedSSBOs = pinnedSSBOs.size();
	for (int i = 0; i < numPinnedSSBOs; i++) {
		if (pinnedSSBOs[i] == SSBO_ID) return;
	}
	pinnedSSBOs.push_back(SSBO_ID);
}

void Cloth::uploadExternalConstraints() {
	GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

	// allocate space for constraints on GPU
	int numConstraints = externalConstraints.size();
	if (numConstraints < 1) return;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sExternalConstraints);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numConstraints * sizeof(glm::vec4),
		NULL, GL_STREAM_COPY);

	// transfer
	glm::vec4 *constraintsMapped = (glm::vec4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, numConstraints * sizeof(glm::vec4), bufMask);

	for (int j = 0; j < numConstraints; j++) {
		constraintsMapped[j] = externalConstraints[j];
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

