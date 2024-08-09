#pragma once
#include <glm/glm.hpp>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <common/utils.hpp>
// holds pointers to everything that can be rendered from a mesh
// - GL buffer for positions
// - index buffer to help with rendering
// - obj loader -> expects quads

using namespace std;


struct ClothData
{
	glm::uvec2 gridsize{ 1000, 1000 };
	glm::vec2 size{ 5.0f, 5.0f };
};


class Mesh
{
public:
  string filename;
  GLuint drawingVAO;
  GLuint sIdxBuffer; // index buffer
  GLuint sPosBuffer; // shader storage buffer object -> holds positions
  //GLuint ssbo_debug; // ssbo for storing debug info

  vector<glm::vec4> initPositions;
  vector<glm::ivec4> QuadsIndices;
  vector<int> indices;

  glm::vec3 color;
  bool isCustomize = false;

  Mesh(string filename);
  Mesh(string filename, glm::vec3 jitter);
  Mesh(const ClothData&);
  ~Mesh();

private:
	glm::vec3 jitter;

  void initialBuffer();
  void buildGeometry();
  void placeToken(string token, ifstream *myfile);

};