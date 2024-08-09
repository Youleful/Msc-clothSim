#include "mesh.hpp"
#include <glm/gtc/matrix_transform.hpp>


Mesh::Mesh(string filename) {
  this->filename = filename;

  /* initialize random seed: */
  srand(time(NULL));

  /* generate secret number between 1 and 100: */
  jitter = glm::vec3(0.0f);

  // load things up
  buildGeometry();

  color = glm::vec3(0.6f);

  initialBuffer();
  
}

Mesh::Mesh(string filename, glm::vec3 jitter) {
	this->filename = filename;

	// compute a randomized, tiny jitter
	/* initialize random seed: */
	srand(time(NULL));

	/* generate secret number between 1 and 100: */
	this->jitter = jitter;

	// load things up
	buildGeometry();

	color = glm::vec3(0.6f);

	initialBuffer();
}

Mesh::Mesh(const ClothData& cloth)
{

	float dx = cloth.size.x / (cloth.gridsize.x - 1);
	float dy = cloth.size.y / (cloth.gridsize.y - 1);
	float du = 1.0f / (cloth.gridsize.x - 1);
	float dv = 1.0f / (cloth.gridsize.y - 1);

	initPositions.resize(cloth.gridsize.x * cloth.gridsize.y);
	glm::mat4 transM = glm::translate(glm::mat4(1.0f), glm::vec3(-cloth.size.x / 2.0f, -cloth.size.y / 2.0f, 2.0f));
	for (uint32_t i = 0; i < cloth.gridsize.y; i++) {
		for (uint32_t j = 0; j < cloth.gridsize.x; j++) {
			initPositions[i + j * cloth.gridsize.y] = transM * glm::vec4(dx * j, dy * i, 0.0f, 1.0f);
		}
	}

	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(0xFFFFFFFF);
	int reset = 0;
	for (uint32_t y = 0; y < cloth.gridsize.y - 1; y++) {
		glm::ivec4 quad;
		for (uint32_t x = 0; x < cloth.gridsize.x; x++) {
			indices.push_back((y + 1) * cloth.gridsize.x + x);
			indices.push_back((y)*cloth.gridsize.x + x);
			if (reset == 0)
			{
				quad.x = (y + 1) * cloth.gridsize.x + x;
				quad.y = (y)*cloth.gridsize.x + x;
				reset = 1;
			}
			else
			{
				quad.z = (y + 1) * cloth.gridsize.x + x;
				quad.w = (y)*cloth.gridsize.x + x;
				QuadsIndices.push_back(quad);
				quad.x = quad.z;
				quad.y = quad.w;
			}
		}
		// Primitive restart (signaled by special value 0xFFFFFFFF)
		indices.push_back(0xFFFFFFFF);
		reset = 0;
	}
	
	initialBuffer();

}

Mesh::~Mesh() {

}

void Mesh::initialBuffer()
{
	glGenVertexArrays(1, &drawingVAO);

	glGenBuffers(1, &sIdxBuffer);

	// upload indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sIdxBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
		&indices[0], GL_STATIC_DRAW);

	// Initialize cloth positions on GPU
	GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	int positionCount = initPositions.size();

	glGenBuffers(1, &sPosBuffer);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sPosBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, positionCount * sizeof(glm::vec4),
		&initPositions[0], GL_STREAM_COPY);

	// bind indices to the VAO.
	glBindVertexArray(drawingVAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sIdxBuffer);

	// try to bind the ssbo to the VAO. this doesn't work yet, see drawMesh in main
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, sPosBuffer);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// shut off the VAO
	glBindVertexArray(0);
}

void Mesh::buildGeometry()
{
  // load the file up
  ifstream inputfile(filename);
  string line;
  if (inputfile.is_open())
  {
    while (getline(inputfile, line))
    {
      placeToken(line, &inputfile);
    }

	inputfile.close();
  }
  return;
}
glm::vec3 parseOneVec3(string token) {
	float x, y, z;
	sscanf_s(token.c_str(), "%f %f %f\n", &x, &y, &z);
	return glm::vec3(x, y, z);
}

glm::ivec4 parseOneivec4(string token)
{
	int x, y, z, w;
	sscanf_s(token.c_str(), "%i %i %i %i\n", &x, &y, &z, &w);
	return glm::ivec4(x, y, z, w);
}

void Mesh::placeToken(string token, ifstream *myfile) {
    if (token.length() == 0)
    {
      ////std::cout << "newline maybe" << std::endl;
      return;
    }

    // case of a vertex
    if (token.compare(0, 1, "v") == 0)
    {
      token.erase(0, 2); // we'll assume v x y z, so erase v and whitespace space
	  initPositions.push_back(glm::vec4(parseOneVec3(token) + jitter, 1.0));
      return;
    }

    // case of a face index. expects quad faces
    if (token.compare(0, 1, "f") == 0)
    {
      token.erase(0, 2); // we'll assume f x y z, so erase f and whitespace space
        glm::ivec4 face_indices = parseOneivec4(token);
		// slight jitter to avoid extreme edge cases

        // objs are 1 indexed
        face_indices -= glm::ivec4(1);
        QuadsIndices.push_back(face_indices);

        // compute triangular indices. quad indices assumed to be cclockwise
        // 0 3
        // 1 2
        glm::ivec3 tri1 = glm::ivec3(face_indices[0], face_indices[1], face_indices[2]);
        glm::ivec3 tri2 = glm::ivec3(face_indices[0], face_indices[2], face_indices[3]);
        indices.push_back(tri1.x);
        indices.push_back(tri1.y);
        indices.push_back(tri1.z);
        indices.push_back(tri2.x);
        indices.push_back(tri2.y);
        indices.push_back(tri2.z);
      return;
    }
    return;
}


