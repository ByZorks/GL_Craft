#include <iostream>
#include <ostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "IndexBuffer.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "VertexArray.h"
#include "VertexBuffer.h"

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"

int main(int argc, char *argv[]) {
    if (!glfwInit())
        return -1;

    GLFWwindow *window = glfwCreateWindow(1280, 720, "GLCraft", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) std::cout << "glewInit() failed" << std::endl;

    std::cout << glGetString(GL_VERSION) << std::endl;

    constexpr float vertices[] = {
        // x, y, z, u, v
        -.5f, .5f, .0f, .0f, 1.0f,
        .5f, .5f, .0f, 1.0f, 1.0f,
        .5f, -.5f, .0f, 1.0f, .0f,
        -.5f, -.5f, .0f , .0f, .0f
    };

    constexpr unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    // Vertex Array Object
    const VertexArray vao;
    const VertexBuffer vbo(vertices, sizeof(vertices));
    VertexBufferLayout layout;
    layout.Push<float>(3); // x, y, z
    layout.Push<float>(2); // u, v
    vao.AddBuffer(vbo, layout);

    // Index Buffer Object
    const IndexBuffer ibo(indices, sizeof(indices));

    // Projection, View, Model matrices
    const glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), // Field of view
        1280.0f / 720.0f, // Aspect ratio
        .1f, // Near plane
        5000.0f // Far plane
    );

    constexpr glm::mat4 view = glm::translate(
        glm::mat4(1.0f), // Identity matrix
        glm::vec3(0.0f, 0.0f, -3.0f) // Translation vector
    );

    const glm::mat4 model = glm::rotate(
        glm::mat4(1.0f), // Identity matrix
        glm::radians(45.0f), // Rotation angle in radians
        glm::vec3(0.0f, 1.0f, 0.0f) // Rotation axis
    );

    const glm::mat4 mvp = projection * view * model;

    // Shader
    Shader shader("../res/shaders/vertex.shader", "../res/shaders/fragment.shader");
    shader.use();
    shader.setUniformMat4f("u_MVP", mvp);

    const Texture grass_texture("../res/textures/grass_side_n.png");
    grass_texture.bind();
    shader.setUniform1i("u_Texture", 0);

    while (!glfwWindowShouldClose(window)) {
        Renderer::Clear();

        Renderer::Draw(vao, ibo);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
