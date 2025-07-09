cmake_minimum_required(VERSION 3.31)
project(GL_Craft)

set(CMAKE_CXX_STANDARD 23)
set(LIBS_DIR "${CMAKE_SOURCE_DIR}/libs")

# Include directories
include_directories(
        ${LIBS_DIR}/glew/include
        ${LIBS_DIR}/glfw/include
)

# Link directories
link_directories(
        ${LIBS_DIR}/glew/lib/Release/x64
        ${LIBS_DIR}/glfw/lib-mingw-w64
)

add_executable(GL_Craft
        src/GLCraft.cpp
        src/Shader.cpp
        src/Shader.cpp
        src/Shader.h
        src/OpenGLDebug.h
        src/OpenGLDebug.cpp
        src/VertexBuffer.h
        src/VertexBuffer.cpp
        src/VertexArray.h
        src/VertexArray.cpp
)

target_compile_definitions(GL_Craft PRIVATE GLEW_STATIC)

# Link OpenGL, GLEW, and GLFW
target_link_libraries(GL_Craft
        glfw3
        glew32s
        opengl32
)