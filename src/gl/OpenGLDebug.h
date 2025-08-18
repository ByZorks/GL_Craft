#ifndef OPENGLDEBUG_H
#define OPENGLDEBUG_H

#ifdef _WIN32
    #define ASSERT(x) if (!(x)) __debugbreak();
#else
    #define ASSERT(x) if (!(x)) __builtin_trap();  // Utilisé sous Linux
#endif
#define GLCall(x) GLClearError();\
x;\
ASSERT(GLLogCall(#x, __FILE__, __LINE__))

void GLClearError();
bool GLLogCall(const char* function, const char* file, int line);

#endif //OPENGLDEBUG_H
