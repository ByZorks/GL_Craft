#include <iostream>
#include <ostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../render/Camera.h"
#include "../render/Renderer.h"
#include "../gl/Shader.h"
#include "../gl/Texture.h"
#include "../world/World.h"

#include "glm.hpp"

#include "imgui.h"
#include "../ui/DebugUI.h"

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

int main(int argc, char *argv[]) {
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "GLCraft", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwMaximizeWindow(window);

    // Camera
    Camera camera(1920, 1080);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(window, &camera);
    glfwSetCursorPosCallback(window, Camera::mouseCallback);

    glfwSwapInterval(0); // Disable VSync

    if (glewInit() != GLEW_OK) std::cout << "glewInit() failed" << std::endl;

    std::cout << glGetString(GL_VERSION) << std::endl; {
        // Shader
        Shader shader("../res/shaders/block.vert", "../res/shaders/block.frag");
        shader.use();
        shader.setUniform1i("u_Texture", 0);

        Shader instanceShader("../res/shaders/instance_vegetation.vert", "../res/shaders/block.frag");
        instanceShader.use();
        instanceShader.setUniform1i("u_Texture", 0);

        Shader waterShader("../res/shaders/water.vert", "../res/shaders/water.frag");
        waterShader.use();
        waterShader.setUniform1i("u_Texture", 0);

        const Texture atlas("../res/textures/atlas/texture_atlas.png");
        atlas.bind();

        // Set up the MVP matrix
        const glm::mat4 projection = camera.getProjectionMatrix();
        constexpr auto model = glm::mat4(1.0f);

        // Debug UI
        DebugUI debugUI(window);
        const ImGuiIO &io = ImGui::GetIO();

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
            Frustum frustum = Camera::getFrustum(mvp);

            // Chunks generation
            world.updateChunks(camera);

            // Render the world
            shader.use();
            shader.setUniformMat4f("u_MVP", mvp);

            unsigned int drawCalls = 0;
            unsigned int visibleChunksCount = 0;
            world.drawChunks(camera, frustum, shader, visibleChunksCount, drawCalls);

            waterShader.use();
            waterShader.setUniformMat4f("u_MVP", mvp);
            world.drawWater(waterShader, drawCalls);

            shader.use();
            shader.setUniformMat4f("u_MVP", mvp);
            unsigned int visibleVegetationsCount = 0;
            world.drawVegetations(camera, frustum, shader, visibleVegetationsCount, drawCalls);

            instanceShader.use();
            instanceShader.setUniformMat4f("u_MVP", mvp);
            world.drawInstances(camera, frustum, visibleVegetationsCount, drawCalls);

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
}
