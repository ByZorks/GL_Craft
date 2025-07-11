#include <iostream>
#include <ostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Camera.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"

#include "glm.hpp"
#include "World.h"
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

    glfwSwapInterval(0); // Disable VSync

    if (glewInit() != GLEW_OK) std::cout << "glewInit() failed" << std::endl;

    std::cout << glGetString(GL_VERSION) << std::endl; {

        World world;
        world.generate();

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

        Renderer::init();
        while (!glfwWindowShouldClose(window)) {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, true);

            Renderer::clear();

            const float deltaTime = Renderer::calculateDeltaTime(static_cast<float>(glfwGetTime()));

            camera.processInput(window, deltaTime);
            glm::mat4 view = glm::lookAt(camera.m_camera_pos(), camera.m_camera_pos() + camera.m_camera_front(),
                                         camera.m_camera_up());
            view = glm::translate(view, glm::vec3(0.0f, -32.0f, 0.0f));
            glm::mat4 mvp = projection * view * model;
            shader.setUniformMat4f("u_MVP", mvp);

            for (auto &chunk : world.m_chunks1()) {
                Renderer::draw(chunk->m_vao(), chunk->m_ibo());
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
