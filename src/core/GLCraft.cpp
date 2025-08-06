#include <iostream>
#include <ostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../gl/Shader.h"
#include "../gl/Texture.h"
#include "../render/Camera.h"
#include "../render/Renderer.h"
#include "../world/World.h"

#include "glm.hpp"

#include "imgui.h"
#include "../ui/DebugUI.h"

constexpr int BASE_WIDTH = 1280;
constexpr int BASE_HEIGHT = 720;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

int main(int argc, char *argv[]) {
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(BASE_WIDTH, BASE_HEIGHT, "GLCraft", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwMaximizeWindow(window);

    // Camera
    Camera camera(BASE_WIDTH, BASE_HEIGHT);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(window, &camera);
    glfwSetCursorPosCallback(window, Camera::mouseCallback);

    glfwSwapInterval(0); // Disable VSync

    if (glewInit() != GLEW_OK) std::cout << "glewInit() failed" << std::endl;

    std::cout << glGetString(GL_VERSION) << std::endl; {
        // Shader
        Shader blockShader("../res/shaders/block.vert", "../res/shaders/block.frag");
        blockShader.use();
        blockShader.setUniform1i("u_Texture", 0);

        Shader grassShader("../res/shaders/grass.vert", "../res/shaders/grass.frag");
        grassShader.use();
        grassShader.setUniform1i("u_Texture", 0);

        Shader waterShader("../res/shaders/water.vert", "../res/shaders/water.frag");
        waterShader.use();
        waterShader.setUniform1i("u_Texture", 0);

        const Texture atlas("../res/textures/atlas/texture_atlas.png");
        atlas.bind();

        // Debug UI
        DebugUI debugUI(window);
        const ImGuiIO &io = ImGui::GetIO();

        World world;
        Renderer::init();
        while (!glfwWindowShouldClose(window)) {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, true);

            // Debug variables
            unsigned int drawCalls = 0;
            unsigned int visibleChunksCount = 0;
            unsigned int visibleVegetationsCount = 0;

            // Handle tab key for UI mode
            debugUI.processInput(window, camera);

            Renderer::clear();
            DebugUI::newFrame();

            // Update camera position and view matrix
            const float deltaTime = Renderer::calculateDeltaTime(static_cast<float>(glfwGetTime()));
            if (camera.m_input_enabled() && !io.WantCaptureKeyboard) {
                camera.processInput(window, deltaTime);
            }
            const glm::mat4 projection = camera.getProjectionMatrix();
            const glm::mat4 view = camera.getViewMatrix();
            const glm::mat4 mvp = projection * view;
            Frustum frustum = Camera::getFrustum(mvp);

            // Chunks generation
            world.updateChunks(camera);

            // Render instances, vegetations, chunks then water
            grassShader.use();
            grassShader.setUniformMat4f("u_MVP", mvp);
            world.drawInstances(camera, frustum, visibleVegetationsCount, drawCalls);

            blockShader.use();
            blockShader.setUniformMat4f("u_MVP", mvp);
            world.drawVegetations(camera, frustum, blockShader, visibleVegetationsCount, drawCalls);

            world.drawChunks(camera, frustum, blockShader, visibleChunksCount, drawCalls);

            waterShader.use();
            waterShader.setUniformMat4f("u_MVP", mvp);
            world.drawWater(waterShader, drawCalls);

            DebugUI::render(visibleChunksCount, visibleVegetationsCount, world.m_loaded_chunks().size(),
                            world.m_loaded_vegetations().size(), drawCalls, camera);
            DebugUI::draw();

            camera.updateLastState();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, const int width, const int height) {
    glViewport(0, 0, width, height);
    const auto camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (!camera) return;
    camera->set_m_aspect_ratio(static_cast<float>(width) / static_cast<float>(height));
}
