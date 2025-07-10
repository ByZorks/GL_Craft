#include "OpenGLDebug.h"

#include <iostream>
#include <GL/glew.h>

void GLClearError() {
    while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, const int line) {
    while (const GLenum error = glGetError()) {
        std::cout << "[OpenGL error] (0x" << std::hex << error << "): " << function << " " << file << ":" << line << std::endl;
        return false;
    }
    return true;
}