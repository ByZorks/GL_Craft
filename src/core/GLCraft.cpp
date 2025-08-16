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
#include "../math/Raycast.h"
#include "../render/HighlightedBlock.h"
#include "../render/PostProcessingMesh.h"
#include "../ui/Crosshair.h"
#include "../ui/DebugUI.h"

static int s_WINDOW_WIDTH = 1280;
static int s_WINDOW_HEIGHT = 720;
static float s_ASPECT_RATIO = static_cast<float>(s_WINDOW_WIDTH) / static_cast<float>(s_WINDOW_HEIGHT);

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

int main(int argc, char *argv[]) {
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(s_WINDOW_WIDTH, s_WINDOW_HEIGHT, "GLCraft", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (glewInit() != GLEW_OK) std::cerr << "glewInit() failed" << std::endl;

    // Camera
    Camera camera(s_WINDOW_WIDTH, s_WINDOW_HEIGHT);
    auto *postProcessingMesh = new PostProcessingMesh(s_WINDOW_WIDTH, s_WINDOW_HEIGHT);
    auto *crosshair = new Crosshair();
    WindowUserPointers pointers = {&camera, postProcessingMesh, crosshair};
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(window, &pointers);
    glfwSetCursorPosCallback(window, Camera::mouseCallback);

    glfwMaximizeWindow(window);
    glfwSwapInterval(0); // Disable VSync

    std::cout << glGetString(GL_VERSION) << std::endl;

    auto *blockShader = new Shader("../res/shaders/block.vert", "../res/shaders/block.frag");
    auto *instancesShader = new Shader("../res/shaders/instances.vert", "../res/shaders/instances.frag");
    auto *waterShader = new Shader("../res/shaders/water.vert", "../res/shaders/water.frag");
    auto *highlightedBlockShader = new Shader("../res/shaders/highlightBlock.vert", "../res/shaders/highlightBlock.frag");
    auto *crosshairShader = new Shader("../res/shaders/crosshair.vert", "../res/shaders/crosshair.frag");
    auto *postProcessingShader = new Shader("../res/shaders/postProcessing.vert", "../res/shaders/postProcessing.frag");
    postProcessingShader->use();
    postProcessingShader->setUniform1i("u_SceneTexture", 0);
    postProcessingShader->setUniform1i("u_DepthTexture", 1);
    postProcessingShader->setUniform1f("u_RenderDistance", Renderer::s_renderDistance);

    const auto *atlas = new Texture("../res/textures/atlas/texture_atlas.png");
    atlas->bind();

    DebugUI debugUI(window);
    const ImGuiIO &io = ImGui::GetIO();

    const auto *highlightedBlock = new HighlightedBlock();

    auto* world = new World();
    Renderer::init();
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, true);

        // Debug variables
        unsigned int drawCalls = 0;
        unsigned int visibleChunksCount = 0;

        // Handle tab key for UI mode
        debugUI.processInput(window, camera);

        postProcessingMesh->getFBO().bind();
        Renderer::clear();
        DebugUI::newFrame();

        // Update camera position and view matrix
        const float deltaTime = Renderer::calculateDeltaTime(static_cast<float>(glfwGetTime()));
        if (camera.isInputEnabled() && !io.WantCaptureKeyboard) {
            camera.processInput(window, deltaTime);
        }
        const glm::mat4 projection = camera.getProjectionMatrix();
        const glm::mat4 view = camera.getViewMatrix();
        const glm::mat4 mvp = projection * view;
        const Frustum frustum = Camera::getFrustum(mvp);

        // Chunks generation
        if (camera.hasCameraChangedChunk()) world->updateChunks(camera);

        // Uniforms
        postProcessingShader->use();
        postProcessingShader->setUniform1b("u_IsUnderWater", camera.isUnderWater(World::getHeight(
                                               static_cast<int>(camera.getCameraPos().x),
                                               static_cast<int>(camera.getCameraPos().z))));

        blockShader->use();
        blockShader->setUniformMat4f("u_MVP", mvp);

        waterShader->use();
        waterShader->setUniformMat4f("u_MVP", mvp);
        waterShader->setUniform1f("u_Time", static_cast<float>(glfwGetTime()));

        instancesShader->use();
        instancesShader->setUniformMat4f("u_MVP", mvp);

        // Render instances, opaques block, transparents blocks then water
        world->drawInstances(drawCalls);

        blockShader->use();
        world->drawChunks(camera, frustum, *blockShader, visibleChunksCount, drawCalls);

        world->drawTransparentChunks(frustum, *blockShader, drawCalls);

        waterShader->use();
        world->drawWater(frustum, *waterShader, drawCalls);

        // Raycasting
        if (std::array<int, 3> selectedBlockCoords = Raycast::castRay(camera.getCameraPos(), camera.getCameraFront(), world->getLoadedChunks());
            selectedBlockCoords[0] != 0 || selectedBlockCoords[1] != 0 || selectedBlockCoords[2] != 0) {
            highlightedBlockShader->use();
            highlightedBlockShader->setUniformMat4f("u_MVP", mvp);
            highlightedBlockShader->setUniform3f("u_Offset",
                static_cast<float>(selectedBlockCoords[0]),
                static_cast<float>(selectedBlockCoords[1]),
                static_cast<float>(selectedBlockCoords[2]));

            highlightedBlock->draw();
            ++drawCalls;
        }

        // Post-processing and crosshair to minimize openGl state changes
        FrameBuffer::unbind();
        postProcessingShader->use();
        postProcessingMesh->getFBO().getColorTexture().bind(0);
        postProcessingMesh->getFBO().getDepthTexture().bind(1);

        Renderer::disableDepthTesting();
        Renderer::draw(postProcessingMesh->getVAO(), postProcessingMesh->getIBO());

        atlas->bind();
        crosshairShader->use();
        crosshairShader->setUniform1f("u_AspectRatio", s_ASPECT_RATIO);
        crosshair->draw();
        Renderer::enableDepthTesting();

        // Render ImGui
        DebugUI::render(visibleChunksCount, world->getLoadedChunks().size(), drawCalls, camera, [world, postProcessingShader, camera] {
            world->updateRenderDistance(*postProcessingShader, camera);
        });
        DebugUI::draw();

        // State update
        camera.updateLastState();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    Raycast::clearCache();
    delete blockShader;
    delete instancesShader;
    delete waterShader;
    delete highlightedBlockShader;
    delete highlightedBlock;
    delete crosshairShader;
    delete crosshair;
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

    pointers->camera->setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    pointers->postProcessingMesh->resize(width, height);

    s_WINDOW_WIDTH = width;
    s_WINDOW_HEIGHT = height;
    s_ASPECT_RATIO = static_cast<float>(width) / static_cast<float>(height);
}
