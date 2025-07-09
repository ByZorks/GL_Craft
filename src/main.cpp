#include <iostream>
#include <ostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "OpenGLDebug.h"
#include "Shader.h"

int main(int argc, char *argv[]) {
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow *window = glfwCreateWindow(1280, 720, "GLCraft", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) std::cout << "glewInit() failed" << std::endl;

    std::cout << glGetString(GL_VERSION) << std::endl;

    constexpr float vertices[] = {
        // x, y, z
        -.5f, .5f, 0.0f,
        .5f, .5f, 0.0f,
        .5f, -.5f, 0.0f,
        -.5f, -.5f, 0.0f
    };

    constexpr unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    unsigned int buffer;
    GLCall(glGenBuffers(1, &buffer));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, buffer));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr));

    unsigned int ibo;
    GLCall(glGenBuffers(1, &ibo));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));

    Shader shader("../res/shaders/vertex.shader", "../res/shaders/fragment.shader");
    shader.use();

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
