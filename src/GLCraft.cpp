#include <algorithm>
#include <iostream>
#include <ostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Camera.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "World.h"

#include "glm.hpp"

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

        // Shader
        Shader shader("../res/shaders/vertex.shader", "../res/shaders/fragment.shader");
        shader.use();

        const Texture atlas("../res/textures/atlas/texture_atlas.png");
        atlas.bind();
        shader.setUniform1i("u_Texture", 0);

        // Set up the MVP matrix
        const glm::mat4 projection = camera.getProjectionMatrix();
        constexpr auto model = glm::mat4(1.0f);

        Renderer::init();
        while (!glfwWindowShouldClose(window)) {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, true);

            Renderer::clear();

            const float deltaTime = Renderer::calculateDeltaTime(static_cast<float>(glfwGetTime()));

            camera.processInput(window, deltaTime);

            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 mvp = projection * view * model;
            shader.setUniformMat4f("u_MVP", mvp);

            Frustum frustum = Camera::getFrustum(mvp);
            std::vector<Chunk * > visibleChunks;
            int visibleChunksCount = 0;
            int totalChunks = 0;
            for (const auto &chunk : world.m_chunks1()) {
                totalChunks++;
                if (frustum.isAABBInFrustum(chunk->m_box1())) {
                    visibleChunks.push_back(chunk);
                    visibleChunksCount++;
                }
            }

            std::ranges::sort(visibleChunks, [camera](const Chunk *a, const Chunk *b) {
                return camera.distanceToCamera(*a) < camera.distanceToCamera(*b);
            });

            for (const auto &chunk : visibleChunks) {
                Renderer::draw(chunk->m_vao(), chunk->m_ibo());
            }

            std::cout << "Visible Chunks: " << visibleChunksCount << " / " << totalChunks << std::endl;


            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
