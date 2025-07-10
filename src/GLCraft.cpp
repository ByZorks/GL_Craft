#include <iostream>
#include <ostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Camera.h"
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

    // Camera
    Camera camera(1280, 720);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(window, &camera);
    glfwSetCursorPosCallback(window, Camera::mouseCallback);

    if (glewInit() != GLEW_OK) std::cout << "glewInit() failed" << std::endl;

    std::cout << glGetString(GL_VERSION) << std::endl; {
        constexpr float vertices[] = {
            // x, y, z, u, v
            // Front face
            -.5f, .5f, .5f, .333f, 1.0f,
            .5f, .5f, .5f, .666f, 1.0f,
            .5f, -.5f, .5f, .666f, .0f,
            -.5f, -.5f, .5f, .333f, .0f,
            // Back face
            -.5f, .5f, -.5f, .333f, 1.0f,
            .5f, .5f, -.5f, .666f, 1.0f,
            .5f, -.5f, -.5f, .666f, .0f,
            -.5f, -.5f, -.5f, .333f, .0f,
            // Left face
            -.5f, .5f, .5f, .333f, 1.0f,
            -.5f, .5f, -.5f, .666f, 1.0f,
            -.5f, -.5f, -.5f, .666f, .0f,
            -.5f, -.5f, .5f, .333f, .0f,
            // Right face
            .5f, .5f, .5f, .333f, 1.0f,
            .5f, .5f, -.5f, .666f, 1.0f,
            .5f, -.5f, -.5f, .666f, .0f,
            .5f, -.5f, .5f, .333f, .0f,
            // Top face
            -.5f, .5f, .5f, .666f, 1.0f,
            .5f, .5f, .5f, 1.0f, 1.0f,
            .5f, .5f, -.5f, 1.0f, .0f,
            -.5f, .5f, -.5f, .666f, .0f,
            // Bottom face
            -.5f, -.5f, .5f, .0f, 1.0f,
            .5f, -.5f, .5f, .333f, 1.0f,
            .5f, -.5f, -.5f, .333f, .0f,
            -.5f, -.5f, -.5f, .0f, .0f,
        };

        constexpr unsigned int indices[] = {
            // Front face
            0, 1, 2,
            2, 3, 0,
            // Back face
            4, 5, 6,
            6, 7, 4,
            // Left face
            8, 9, 10,
            10, 11, 8,
            // Right face
            12, 13, 14,
            14, 15, 12,
            // Top face
            16, 17, 18,
            18, 19, 16,
            // Bottom face
            20, 21, 22,
            22, 23, 20
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

        constexpr auto model = glm::mat4(1.0f); // Identity matrix for model

        // Shader
        Shader shader("../res/shaders/vertex.shader", "../res/shaders/fragment.shader");
        shader.use();

        const Texture atlas("../res/textures/atlas/texture_atlas.png");
        atlas.bind();
        shader.setUniform1i("u_Texture", 0);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        while (!glfwWindowShouldClose(window)) {
            Renderer::Clear();

            float deltaTime = Renderer::calculateDeltaTime(static_cast<float>(glfwGetTime()));

            camera.processInput(window, deltaTime);
            glm::mat4 view = glm::lookAt(camera.m_camera_pos(), camera.m_camera_pos() + camera.m_camera_front(),
                                         camera.m_camera_up());
            glm::mat4 mvp = projection * view * model;
            shader.setUniformMat4f("u_MVP", mvp);

            Renderer::Draw(vao, ibo);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
