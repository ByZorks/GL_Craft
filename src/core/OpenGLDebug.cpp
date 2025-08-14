#include "../core/OpenGLDebug.h"

#include <iostream>
#include <GL/glew.h>

void GLClearError() {
    while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, const int line) {
    GLenum error;
    bool noError = true;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cerr << "[OpenGL error] (0x" << std::hex << error << "): " << function << " " << file << ":" << line << std::endl;
        noError = false;
    }
    return noError;
}