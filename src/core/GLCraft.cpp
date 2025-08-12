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
#include "WindowUserPointers.h"
#include "../gl/FrameBuffer.h"
#include "../render/PostProcessingMesh.h"
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

    if (glewInit() != GLEW_OK) std::cerr << "glewInit() failed" << std::endl;

    // Camera
    Camera camera(BASE_WIDTH, BASE_HEIGHT);
    auto *postProcessingMesh = new PostProcessingMesh(BASE_WIDTH, BASE_HEIGHT);
    WindowUserPointers pointers = {&camera, postProcessingMesh};
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(window, &pointers);
    glfwSetCursorPosCallback(window, Camera::mouseCallback);

    glfwMaximizeWindow(window);
    glfwSwapInterval(0); // Disable VSync

    std::cout << glGetString(GL_VERSION) << std::endl;

    auto *blockShader = new Shader("../res/shaders/block.vert", "../res/shaders/block.frag");
    auto *grassShader = new Shader("../res/shaders/grass.vert", "../res/shaders/grass.frag");
    auto *waterShader = new Shader("../res/shaders/water.vert", "../res/shaders/water.frag");
    auto *postProcessingShader = new Shader("../res/shaders/postProcessing.vert", "../res/shaders/postProcessing.frag");

    const auto *atlas = new Texture("../res/textures/atlas/texture_atlas.png");
    atlas->bind();

    DebugUI debugUI(window);
    const ImGuiIO &io = ImGui::GetIO();

    auto* world = new World();
    Renderer::init();
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, true);

        // Debug variables
        unsigned int drawCalls = 0;
        unsigned int visibleChunksCount = 0;

        // Handle tab key for UI mode
        debugUI.processInput(window, camera);

        postProcessingMesh->m_fbo().bind();
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
        world->updateChunks(camera);

        // Render instances, opaques block, transparents blocks then water
        grassShader->use();
        grassShader->setUniformMat4f("u_MVP", mvp);
        world->drawInstances(drawCalls);

        blockShader->use();
        blockShader->setUniformMat4f("u_MVP", mvp);
        world->drawChunks(camera, frustum, *blockShader, visibleChunksCount, drawCalls);

        world->drawTransparentChunks(*blockShader, drawCalls);

        waterShader->use();
        waterShader->setUniformMat4f("u_MVP", mvp);
        world->drawWater(*waterShader, drawCalls);

        // Post-processing
        FrameBuffer::unbind();
        postProcessingShader->use();
        postProcessingShader->setUniform1f("u_RenderDistance", Renderer::m_renderDistance);
        postProcessingShader->setUniform1b("u_IsUnderWater", camera.isUnderWater(world->getHeight(
                                               static_cast<int>(camera.m_camera_pos().x),
                                               static_cast<int>(camera.m_camera_pos().z))));

        postProcessingShader->setUniform1i("u_SceneTexture", 0);
        postProcessingMesh->m_fbo().m_color_texture().bind(0);

        postProcessingShader->setUniform1i("u_DepthTexture", 1);
        postProcessingMesh->m_fbo().m_depth_texture().bind(1);

        Renderer::disableDepthTesting();
        Renderer::draw(postProcessingMesh->m_vao(), postProcessingMesh->m_ibo());
        Renderer::enableDepthTesting();

        // Render ImGui
        DebugUI::render(visibleChunksCount, world->m_loaded_chunks().size(), drawCalls, camera, [world] {
            world->updateRenderDistance();
        });
        DebugUI::draw();

        // State update
        atlas->bind();
        camera.updateLastState();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete blockShader;
    delete grassShader;
    delete waterShader;
    delete postProcessingShader;
    delete postProcessingMesh;
    delete world;

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, const int width, const int height) {
    glViewport(0, 0, width, height);
    const auto pointers = static_cast<WindowUserPointers *>(glfwGetWindowUserPointer(window));
    if (!pointers) return;

    pointers->camera->set_m_aspect_ratio(static_cast<float>(width) / static_cast<float>(height));
    pointers->postProcessingMesh->resize(width, height);
}
