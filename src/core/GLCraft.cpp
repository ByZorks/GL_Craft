#include <algorithm>
#include <iostream>
#include <ostream>
#include <ranges>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../render/Camera.h"
#include "../render/Renderer.h"
#include "../render/Shader.h"
#include "../render/Texture.h"
#include "../world/World.h"

#include "glm.hpp"

#include "imgui.h"
#include "../ui/DebugUI.h"

int main(int argc, char *argv[]) {
    if (!glfwInit())
        return -1;

    GLFWwindow *window = glfwCreateWindow(1920, 1080, "GLCraft", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Camera
    Camera camera(1920, 1080);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(window, &camera);
    glfwSetCursorPosCallback(window, Camera::mouseCallback);

    glfwSwapInterval(0); // Disable VSync

    if (glewInit() != GLEW_OK) std::cout << "glewInit() failed" << std::endl;

    std::cout << glGetString(GL_VERSION) << std::endl;

    {
        // Shader
        Shader shader("../res/shaders/block.vert", "../res/shaders/block.frag");
        shader.use();

        const Texture atlas("../res/textures/atlas/texture_atlas.png");
        atlas.bind();
        shader.setUniform1i("u_Texture", 0);

        // Set up the MVP matrix
        const glm::mat4 projection = camera.getProjectionMatrix();
        constexpr auto model = glm::mat4(1.0f);

        // Debug UI
        DebugUI debugUI(window);
        const ImGuiIO& io = ImGui::GetIO();

        float renderDistance = 16.0f * static_cast<float>(Chunk::m_size1()); // Render distance in blocks
        World world;
        Renderer::init();
        while (!glfwWindowShouldClose(window)) {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, true);

            // Handle tab key for UI mode
            debugUI.processInput(window, camera);

            Renderer::clear();
            DebugUI::newFrame();

            // Update camera position and view matrix
            const float deltaTime = Renderer::calculateDeltaTime(static_cast<float>(glfwGetTime()));
            if (camera.m_input_enabled() && !io.WantCaptureKeyboard) {
                camera.processInput(window, deltaTime);
            }
            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 mvp = projection * view * model;
            shader.setUniformMat4f("u_MVP", mvp);

            // Chunks generation
            auto t1 = std::chrono::high_resolution_clock::now();
            world.updateChunks(camera, renderDistance);
            auto t2 = std::chrono::high_resolution_clock::now();

            auto ms_int = duration_cast<std::chrono::milliseconds>(t2 - t1);
            if (ms_int.count() > 0) std::cout << "[updateChunks] " << ms_int.count() << "ms\n";

            // Render the world
            Frustum frustum = Camera::getFrustum(mvp);
            unsigned int visibleChunksCount = 0;
            world.forEachRenderableChunk([&](const Chunk *chunk) {
                if (camera.distanceToCamera(*chunk) > renderDistance) return;
                if (frustum.isAABBInFrustum(chunk->m_box1())) {
                    Renderer::draw(chunk->m_vao(), chunk->m_ibo());
                    visibleChunksCount++;
                }
            });

            if (debugUI.isUIMode()) {
                unsigned int totalChunks = world.getChunksToRender().size();
                DebugUI::render(visibleChunksCount, totalChunks, renderDistance, camera);
            }
            DebugUI::draw();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
