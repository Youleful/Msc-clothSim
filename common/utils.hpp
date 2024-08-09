#ifndef UTILS_HPP
#define UTILS_HPP
#include <GL/glew.h>

bool loadBMP_custom(const char* imagepath, int& width, int& height, unsigned char* &data);


namespace glslUtility {
    char* loadFile(const char* fname, GLint& fSize);

    bool readAndCompileShader(const char* shader_path, const GLuint& id);

    bool readAndCompileComputeShader(const char* shader_path, const GLuint& id);

    GLuint LoadShaders(const char* vertex_file_path, const char*
        fragment_file_path, const char* attributeLocations[], GLuint numberOfLocations);

    GLuint LoadCompShaders(const char* compute_file_path);

    GLuint LoadGeomShaders(const char* vertex_file_path, const char* geometry_file_path, const char* fragment_file_path, const char* attributeLocations[], GLuint numberOfLocations);
}

#endif