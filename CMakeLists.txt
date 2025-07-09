cmake_minimum_required(VERSION 3.31)
project(GL_Craft)

set(CMAKE_CXX_STANDARD 23)

include_directories(
        libs/glfw/include
        libs/glew/include
)

link_directories(
        libs/glfw/lib-mingw-w64
        libs/glew/lib/Release/x64
)

add_executable(GL_Craft src/main.cpp)

target_link_libraries(GL_Craft
        glfw3
        glew32
        opengl32
)
