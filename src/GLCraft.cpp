#include <iostream>
#include <ostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "IndexBuffer.h"
#include "OpenGLDebug.h"
#include "Shader.h"
#include "VertexArray.h"
#include "VertexBuffer.h"

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

    // Vertex Array Object
    const VertexArray vao;
    const VertexBuffer vbo(vertices, sizeof(vertices));
    VertexBufferLayout layout;
    layout.Push<float>(3); // 3 floats per vertex (x, y, z)
    vao.AddBuffer(vbo, layout);

    // Index Buffer Object
    const IndexBuffer ibo(indices, sizeof(indices));

    // Shader
    Shader shader("../res/shaders/vertex.shader", "../res/shaders/fragment.shader");
    shader.use();
    GLCall(const int location = glGetUniformLocation(shader.m_program_id(), "u_Color"));
    GLCall(glUniform4f(location, 0.2f, 0.3f, 0.8f, 1.0f));

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        vao.Bind();
        ibo.Bind();
        GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
