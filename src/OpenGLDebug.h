#ifndef OPENGLDEBUG_H
#define OPENGLDEBUG_H

#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
x;\
ASSERT(GLLogCall(#x, __FILE__, __LINE__))

void GLClearError();
bool GLLogCall(const char* function, const char* file, const int line);

#endif //OPENGLDEBUG_H
