cmake_minimum_required(VERSION 3.31)
project(GL_Craft)

set(CMAKE_CXX_STANDARD 23)
set(LIBS_DIR "${CMAKE_SOURCE_DIR}/libs")

include_directories(
        ${LIBS_DIR}/glew/include
        ${LIBS_DIR}/glfw/include
        ${LIBS_DIR}/stb
        ${LIBS_DIR}/glm
)

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
        src/IndexBuffer.cpp
        src/IndexBuffer.h
        src/Renderer.cpp
        src/Renderer.h
        src/Texture.cpp
        src/Texture.h
        src/Camera.cpp
        src/Camera.h
        src/Block.cpp
        src/Block.h
        src/Chunk.cpp
        src/Chunk.h
        src/World.cpp
        src/World.h
)

target_compile_definitions(GL_Craft PRIVATE GLEW_STATIC)

target_link_libraries(GL_Craft
        glfw3
        glew32s
        opengl32
)