#include <iostream>
using namespace std;
#define WORK_GROUP_SIZE_X 1024
#define WORK_GROUP_SIZE_Y 1

#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "utils.hpp"

bool loadBMP_custom(const char* imagepath, int& width, int& height, unsigned char* &data) {

	cout << "Reading image " << imagepath;

	// Data read from the header of the BMP file
	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	// Actual RGB data

	// Open the file
	FILE* file = fopen(imagepath, "rb");
	if (!file) {
		cout << imagepath << "could not be opened.Are you in the right directory ?";
		return false;
	}

	// Read the header, i.e. the 54 first bytes
	// If less than 54 bytes are read, problem
	if (fread(header, 1, 54, file) != 54) {
		cout <<"Not a correct BMP file"<<endl;
		fclose(file);
		return false;
	}
	// A BMP files always begins with "BM"
	if (header[0] != 'B' || header[1] != 'M') {
		cout<< "Not a correct BMP file"<<endl;
		fclose(file);
		return false;
	}
	// Make sure this is a 24bpp file
	if (*(int*)&(header[0x1E]) != 0) {cout << "Not a correct BMP file" << endl;  fclose(file); return 0; }
	if (*(int*)&(header[0x1C]) != 24) {cout << "Not a correct BMP file" << endl;    fclose(file); return 0; }

	// Read the information about the image
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = width * height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way

	// Create a buffer
	data = new unsigned char[imageSize];

	// Read the actual data from the file into the buffer
	fseek(file, dataPos, SEEK_SET);
	fread(data, 1, imageSize, file);

	// Everything is in memory now, the file can be closed.
	fclose(file);
	return true;
}

/**
 * @file      glslUtility.cpp
 * @brief     A utility namespace for loading GLSL shaders.
 * @authors   Varun Sampath, Patrick Cozzi, Karl Li
 * @date      2012
 * @copyright University of Pennsylvania
 */


using std::ios;

namespace glslUtility {
    typedef struct {
        GLint vertex;
        GLint fragment;
        GLint geometry;
    } shaders_t;

    char* loadFile(const char* fname, GLint& fSize) {
        // file read based on example in cplusplus.com tutorial
        std::ifstream file(fname, ios::in | ios::binary | ios::ate);
        if (file.is_open()) {
            unsigned int size = (unsigned int)file.tellg();
            fSize = size;
            char* memblock = new char[size];
            file.seekg(0, ios::beg);
            file.read(memblock, size);
            file.close();
            std::cout << "file " << fname << " loaded" << std::endl;
            return memblock;
        }

        std::cout << "Unable to open file " << fname << std::endl;
        exit(1);
    }

    // printShaderInfoLog
    // From OpenGL Shading Language 3rd Edition, p215-216
    // Display (hopefully) useful error messages if shader fails to compile
    void printShaderInfoLog(GLint shader) {
        int infoLogLen = 0;
        int charsWritten = 0;
        GLchar* infoLog;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

        if (infoLogLen > 1) {
            infoLog = new GLchar[infoLogLen];
            // error check for fail to allocate memory omitted
            glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog);
            std::cout << "InfoLog:" << std::endl << infoLog << std::endl;
            delete[] infoLog;
        }
    }

    void printLinkInfoLog(GLint prog) {
        int infoLogLen = 0;
        int charsWritten = 0;
        GLchar* infoLog;

        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLen);

        if (infoLogLen > 1) {
            infoLog = new GLchar[infoLogLen];
            // error check for fail to allocate memory omitted
            glGetProgramInfoLog(prog, infoLogLen, &charsWritten, infoLog);
            std::cout << "InfoLog:" << std::endl << infoLog << std::endl;
            delete[] infoLog;
        }
    }

    shaders_t loadShaders(const char* vert_path, const char* geom_path, const char* frag_path) {
        GLint f, g = -1, v;

        char* vs, * gs, * fs;

        v = glCreateShader(GL_VERTEX_SHADER);
        if (geom_path) {
            g = glCreateShader(GL_GEOMETRY_SHADER);
        }
        f = glCreateShader(GL_FRAGMENT_SHADER);

        // load shaders & get length of each
        GLint vlen;
        GLint glen;
        GLint flen;
        vs = loadFile(vert_path, vlen);
        if (geom_path) {
            gs = loadFile(geom_path, glen);
        }
        fs = loadFile(frag_path, flen);

        const char* vv = vs;
        const char* gg = geom_path ? gs : NULL;
        const char* ff = fs;

        glShaderSource(v, 1, &vv, &vlen);
        if (geom_path) {
            glShaderSource(g, 1, &gg, &glen);
        }
        glShaderSource(f, 1, &ff, &flen);

        GLint compiled;

        glCompileShader(v);
        glGetShaderiv(v, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            std::cout << "Vertex shader not compiled." << std::endl;
        }
        printShaderInfoLog(v);

        if (geom_path) {
            glCompileShader(g);
            glGetShaderiv(g, GL_COMPILE_STATUS, &compiled);
            if (!compiled) {
                std::cout << "Geometry shader not compiled." << std::endl;
            }
            printShaderInfoLog(g);
        }

        glCompileShader(f);
        glGetShaderiv(f, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            std::cout << "Fragment shader not compiled." << std::endl;
        }
        printShaderInfoLog(f);

        shaders_t out;
        out.vertex = v;
        out.geometry = g;
        out.fragment = f;

        delete[] vs; // dont forget to free allocated memory, or else really bad things start happening
        if (geom_path) {
            delete[] gs;
        }
        delete[] fs; // we allocated this in the loadFile function...

        return out;
    }

    void attachAndLinkProgram(GLuint program, shaders_t shaders) {
        glAttachShader(program, shaders.vertex);
        if (shaders.geometry >= 0) {
            glAttachShader(program, shaders.geometry);
        }
        glAttachShader(program, shaders.fragment);

        glLinkProgram(program);
        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            std::cout << "Program did not link." << std::endl;
        }
        printLinkInfoLog(program);
    }

    GLuint createProgram(
        const char* vertexShaderPath,
        const char* fragmentShaderPath,
        const char* attributeLocations[], GLuint numberOfLocations) {
        glslUtility::shaders_t shaders = glslUtility::loadShaders(vertexShaderPath, NULL, fragmentShaderPath);

        GLuint program = glCreateProgram();

        for (GLuint i = 0; i < numberOfLocations; ++i) {
            glBindAttribLocation(program, i, attributeLocations[i]);
        }

        glslUtility::attachAndLinkProgram(program, shaders);

        return program;
    }

    GLuint createProgram(
        const char* vertexShaderPath,
        const char* geometryShaderPath,
        const char* fragmentShaderPath,
        const char* attributeLocations[], GLuint numberOfLocations) {
        glslUtility::shaders_t shaders = glslUtility::loadShaders(vertexShaderPath, geometryShaderPath, fragmentShaderPath);

        GLuint program = glCreateProgram();

        for (GLuint i = 0; i < numberOfLocations; ++i) {
            glBindAttribLocation(program, i, attributeLocations[i]);
        }

        glslUtility::attachAndLinkProgram(program, shaders);

        return program;
    }


    bool readAndCompileShader(const char* shader_path, const GLuint& id) {
        std::string shaderCode;
        std::ifstream shaderStream(shader_path, std::ios::in);
        if (shaderStream.is_open()) {
            std::stringstream sstr;
            sstr << shaderStream.rdbuf();
            shaderCode = sstr.str();
            shaderStream.close();
        }
        else {
            std::cout << "Impossible to open " << shader_path << ". Are you in the right directory ? " << std::endl;
            return false;
        }

        std::cout << "Compiling shader :" << shader_path << std::endl;
        char const* sourcePointer = shaderCode.c_str();
        glShaderSource(id, 1, &sourcePointer, NULL);
        glCompileShader(id);
        GLint Result = GL_FALSE;
        int InfoLogLength;
        glGetShaderiv(id, GL_COMPILE_STATUS, &Result);
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &InfoLogLength);
        if (InfoLogLength > 0) {
            std::vector<char> shaderErrorMessage(InfoLogLength + 1);
            glGetShaderInfoLog(id, InfoLogLength, NULL, &shaderErrorMessage[0]);
            std::cout << &shaderErrorMessage[0] << std::endl;
        }
        std::cout << "Compilation of Shader: " << shader_path << " " << (Result == GL_TRUE ? "Success" :
            "Failed!") << std::endl;
        return Result == 1;
    }

    bool replace(std::string& str, const std::string& from, const std::string& to) {
        size_t start_pos = str.find(from);
        if (start_pos == std::string::npos)
            return false;
        str.replace(start_pos, from.length(), to);
        return true;
    }


    bool readAndCompileComputeShader(const char* shader_path, const GLuint& id)
    {
        std::string shaderCode;
        std::ifstream shaderStream(shader_path, std::ios::in);
        if (shaderStream.is_open()) {
            std::stringstream sstr;
            sstr << shaderStream.rdbuf();
            shaderCode = sstr.str();
            shaderStream.close();
        }
        else {
            std::cout << "Impossible to open " << shader_path << ". Are you in the right directory ? " << std::endl;
            return false;
        }

        // check and edit the shader so the workgroup size is correct
        replace(shaderCode, "WORK_GROUP_SIZE_X XX", "WORK_GROUP_SIZE_X " + std::to_string(WORK_GROUP_SIZE_X));
        replace(shaderCode, "WORK_GROUP_SIZE_Y XX", "WORK_GROUP_SIZE_Y " + std::to_string(WORK_GROUP_SIZE_Y));

        char* cs_str_edited = new char[shaderCode.length() + 1];
        strcpy(cs_str_edited, shaderCode.c_str());

        std::cout << "Compiling shader :" << shader_path << std::endl;
        char const* sourcePointer = shaderCode.c_str();
        int cs_len = shaderCode.length();

        glShaderSource(id, 1, &cs_str_edited, &cs_len);
        glCompileShader(id);
        GLint Result = GL_FALSE;
        int InfoLogLength;
        glGetShaderiv(id, GL_COMPILE_STATUS, &Result);
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &InfoLogLength);
        if (InfoLogLength > 0) {
            std::vector<char> shaderErrorMessage(InfoLogLength + 1);
            glGetShaderInfoLog(id, InfoLogLength, NULL, &shaderErrorMessage[0]);
            std::cout << &shaderErrorMessage[0] << std::endl;
        }
        std::cout << "Compilation of Shader: " << shader_path << " " << (Result == GL_TRUE ? "Success" :
            "Failed!") << std::endl;
        return Result == 1;
    }

    GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path, const char* attributeLocations[], GLuint numberOfLocations)
    {
        GLuint program = glCreateProgram();

        for (GLuint i = 0; i < numberOfLocations; ++i) {
            glBindAttribLocation(program, i, attributeLocations[i]);
        }

        // Create the shaders - tasks 1 and 2 
        GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
        GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
        bool vok = readAndCompileShader(vertex_file_path, VertexShaderID);
        bool fok = readAndCompileShader(fragment_file_path, FragmentShaderID);
        if (vok && fok) {
            GLint Result = GL_FALSE;
            int InfoLogLength;
            std::cout << "Linking program" << std::endl;
            program = glCreateProgram();
            glAttachShader(program, VertexShaderID);
            glAttachShader(program, FragmentShaderID);
            glLinkProgram(program);
            glGetProgramiv(program, GL_LINK_STATUS, &Result);
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &InfoLogLength);
            if (InfoLogLength > 0) {
                std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
                glGetProgramInfoLog(program, InfoLogLength, NULL, &ProgramErrorMessage[0]);
                std::cout << &ProgramErrorMessage[0];
            }
            std::cout << "Linking program: " << (Result == GL_TRUE ? "Success" : "Failed!") <<
                std::endl;
        }
        else {
            std::cout << "Program will not be linked: one of the shaders has an error" <<
                std::endl;
        }
        glDeleteShader(VertexShaderID);
        glDeleteShader(FragmentShaderID);

        return program;
    }

    GLuint LoadCompShaders(const char* compute_file_path)
    {
        GLuint program = glCreateProgram();
        GLuint ComputeShaderID = glCreateShader(GL_COMPUTE_SHADER);

        bool cok = readAndCompileComputeShader(compute_file_path, ComputeShaderID);


        if (cok) {
            GLint Result = GL_FALSE;
            int InfoLogLength;
            std::cout << "Linking program" << std::endl;
            program = glCreateProgram();
            glAttachShader(program, ComputeShaderID);
            glLinkProgram(program);
            glGetProgramiv(program, GL_LINK_STATUS, &Result);
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &InfoLogLength);
            if (InfoLogLength > 0) {
                std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
                glGetProgramInfoLog(program, InfoLogLength, NULL, &ProgramErrorMessage[0]);
                std::cout << &ProgramErrorMessage[0];
            }
            std::cout << "Linking program: " << (Result == GL_TRUE ? "Success" : "Failed!") <<
                std::endl;
        }
        else {
            std::cout << "Program will not be linked: one of the shaders has an error" <<
                std::endl;
        }
        glDeleteShader(ComputeShaderID);

        return program;
    }


    GLuint LoadGeomShaders(const char* vertex_file_path, const char* geometry_file_path, const char* fragment_file_path, const char* attributeLocations[], GLuint numberOfLocations)
    {
        GLuint program = glCreateProgram();

        for (GLuint i = 0; i < numberOfLocations; ++i) {
            glBindAttribLocation(program, i, attributeLocations[i]);
        }

        // Create the shaders - tasks 1 and 2 
        GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
        GLuint GeometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);
        GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
        bool vok = readAndCompileShader(vertex_file_path, VertexShaderID);
        bool gok = readAndCompileShader(geometry_file_path, GeometryShaderID);
        bool fok = readAndCompileShader(fragment_file_path, FragmentShaderID);
        if (vok && fok && gok) {
            GLint Result = GL_FALSE;
            int InfoLogLength;
            std::cout << "Linking program" << std::endl;
            program = glCreateProgram();
            glAttachShader(program, VertexShaderID);
            glAttachShader(program, GeometryShaderID);
            glAttachShader(program, FragmentShaderID);
            glLinkProgram(program);
            glGetProgramiv(program, GL_LINK_STATUS, &Result);
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &InfoLogLength);
            if (InfoLogLength > 0) {
                std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
                glGetProgramInfoLog(program, InfoLogLength, NULL, &ProgramErrorMessage[0]);
                std::cout << &ProgramErrorMessage[0];
            }
            std::cout << "Linking program: " << (Result == GL_TRUE ? "Success" : "Failed!") <<
                std::endl;
        }
        else {
            std::cout << "Program will not be linked: one of the shaders has an error" <<
                std::endl;
        }
        glDeleteShader(VertexShaderID);
        glDeleteShader(GeometryShaderID);
        glDeleteShader(FragmentShaderID);

        return program;
    }

}
