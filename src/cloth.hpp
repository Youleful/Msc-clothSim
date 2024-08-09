#pragma once
#include "mesh.hpp"

#define NUM_INT_CON_BUFFERS 8 // number of internal constraint buffers

struct ClothAttri
{
	float internal_K = 0.9f;
	float pin_K = 1.0f;
	float inv_mass = 441.0f;
	float static_constraint_bounce = 0.1f;
};


class Cloth : public Mesh
{
public:
	int numInternalConstraintBuffers = NUM_INT_CON_BUFFERS;

// positions will be vec4s: x, y, z, mass
// predicted positions will also be vec4s: x, y, z, invMass

  //storage for velocity
  GLuint sVelBuffer; 
  GLuint sDebugBuffer;

  //two prediction position
  GLuint sPrevPredPosBuffer;
  GLuint sCurPredPosBuffer;
  // all constraints in these buffers are vec4s:
  // index of pos to modify, index of influencer, rest length, stiffness K
  // a negative rest length will indicate a "pin" constraint
  GLuint sInternalConstraints[NUM_INT_CON_BUFFERS];
  GLuint sExternalConstraints;

  GLuint sCollisionConstraints;

  // these vec3 constraints are index, index, rest length
  std::vector<glm::vec4> internalConstraints[NUM_INT_CON_BUFFERS];
  std::vector<glm::vec4> externalConstraints; // pin

  std::vector<GLuint> pinnedSSBOs; // SSBOs that this is pinned to


  ClothAttri clothAttri;

  Cloth(string filename, glm::vec3 jitter);
  Cloth(const ClothData&);
  ~Cloth();
  void addPinConstraint(int thisIdx, int otherIdx, GLuint SSBO_ID);
  void uploadExternalConstraints(); // upload all determined constraints.

private:
  void generateConstraints();
  void initialClothBuffer();
};