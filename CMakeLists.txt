cmake_minimum_required(VERSION 3.31)
project(GL_Craft)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto")
set(LIBS_DIR "${CMAKE_SOURCE_DIR}/libs")
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/libs/imgui)

include_directories(
        ${LIBS_DIR}/glew/include
        ${LIBS_DIR}/glfw/include
        ${LIBS_DIR}/stb
        ${LIBS_DIR}/glm
        ${LIBS_DIR}/imgui
        ${LIBS_DIR}/fastNoiseLite
)

link_directories(
        ${LIBS_DIR}/glew/lib/Release/x64
        ${LIBS_DIR}/glfw/lib-mingw-w64
)

add_executable(GL_Craft
        src/core/GLCraft.cpp
        src/gl/Shader.cpp
        src/gl/Shader.cpp
        src/gl/Shader.h
        src/core/OpenGLDebug.h
        src/core/OpenGLDebug.cpp
        src/gl/VertexBuffer.h
        src/gl/VertexBuffer.cpp
        src/gl/VertexArray.h
        src/gl/VertexArray.cpp
        src/gl/IndexBuffer.cpp
        src/gl/IndexBuffer.h
        src/render/Renderer.cpp
        src/render/Renderer.h
        src/gl/Texture.cpp
        src/gl/Texture.h
        src/render/Camera.cpp
        src/render/Camera.h
        src/world/Block.cpp
        src/world/Block.h
        src/world/Chunk.cpp
        src/world/Chunk.h
        src/world/World.cpp
        src/world/World.h
        src/math/Frustum.cpp
        src/math/Frustum.h
        src/math/Plane.cpp
        src/math/Plane.h
        src/math/AABB.cpp
        src/math/AABB.h
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/imgui_impl_opengl3.cpp
        src/ui/DebugUI.cpp
        src/ui/DebugUI.h
        src/utils/ThreadSafeQueue.h
        src/utils/CustomHash.h
        src/render/ThreadPool.h
        src/render/ThreadPool.cpp
        src/world/vegetations/trees/Tree.cpp
        src/world/Mesh.h
        src/world/vegetations/trees/Tree.h
        src/world/Mesh.cpp
        src/world/vegetations/grass/ShortGrass.cpp
        src/world/vegetations/grass/ShortGrass.h
        src/world/vegetations/grass/GrassInstanceRenderer.cpp
        src/world/vegetations/grass/GrassInstanceRenderer.h
)

target_compile_definitions(GL_Craft PRIVATE GLEW_STATIC)

target_link_libraries(GL_Craft
        glfw3
        glew32s
        opengl32
)