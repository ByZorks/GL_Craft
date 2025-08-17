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

static bool s_LEFT_CLICKED = false;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);

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
    glfwSetMouseButtonCallback(window, mouse_button_callback);

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
        waterShader->setUniform3f("u_CameraPos", camera.getCameraPos().x,
                                  camera.getCameraPos().y,
                                  camera.getCameraPos().z);

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
        if (auto [chunk, blockLocalPosition, blockWorldPosition, hitBlock, type] = Raycast::castRay(camera.getCameraPos(), camera.getCameraFront(), world->getLoadedChunks());
            hitBlock) {
            // Highlight the block
            highlightedBlockShader->use();
            highlightedBlockShader->setUniformMat4f("u_MVP", mvp);
            highlightedBlockShader->setUniform3f("u_Offset",
                static_cast<float>(blockWorldPosition[0]),
                static_cast<float>(blockWorldPosition[1]),
                static_cast<float>(blockWorldPosition[2]));

            highlightedBlock->draw();
            ++drawCalls;

            // Handle left click to break
            // TODO: Implement partial mesh update
            if (s_LEFT_CLICKED && camera.isInputEnabled()) {

                world->getThreadPool().enqueue_no_future([chunk, blockLocalPosition, world, type] {
                    const auto t1 = std::chrono::high_resolution_clock::now();

                    // Update adjacents chunks if the block is at the border of the chunk
                    const bool isAtLeftBorder = blockLocalPosition[0] == 0;
                    const bool isAtRightBorder = blockLocalPosition[0] + 1 == Chunk::SIZE;
                    const bool isAtBottomBorder = blockLocalPosition[1] == 0;
                    const bool isAtTopBorder = blockLocalPosition[1] + 1 == Chunk::SIZE;
                    const bool isAtFrontBorder = blockLocalPosition[2] == 0;
                    const bool isAtBackBorder = blockLocalPosition[2] + 1 == Chunk::SIZE;

                    // Retrieve adjacent(s) chunk(s) based on face at chunk border
                    constexpr int chunkSize = Chunk::SIZE;
                    constexpr int minBlockPositionInAdjacentChunk = -1; // chunk will add +1 when accessing the block
                    constexpr int maxBlockPositionInAdjacentChunk = Chunk::SIZE; // chunk will add +1 when accessing the block
                    if (isAtLeftBorder) {
                        const int leftChunkX = chunk->getX() - chunkSize;
                        const int leftChunkY = chunk->getY();
                        const int leftChunkZ = chunk->getZ();
                        if (const std::shared_ptr<Chunk> adjacentChunk = world->getChunk(leftChunkX, leftChunkY, leftChunkZ)) {
                            adjacentChunk->deleteBlock(maxBlockPositionInAdjacentChunk, blockLocalPosition[1], blockLocalPosition[2], type);
                            world->getMeshesToUpdate().push(adjacentChunk);
                        }
                    }
                    if (isAtRightBorder) {
                        const int rightChunkX = chunk->getX() + chunkSize;
                        const int rightChunkY = chunk->getY();
                        const int rightChunkZ = chunk->getZ();
                        if (const std::shared_ptr<Chunk> adjacentChunk = world->getChunk(rightChunkX, rightChunkY, rightChunkZ)) {
                            adjacentChunk->deleteBlock(minBlockPositionInAdjacentChunk, blockLocalPosition[1], blockLocalPosition[2], type);
                            world->getMeshesToUpdate().push(adjacentChunk);
                        }
                    }
                    if (isAtBottomBorder) {
                        const int bottomChunkX = chunk->getX();
                        const int bottomChunkY = chunk->getY() - chunkSize;
                        const int bottomChunkZ = chunk->getZ();
                        if (const std::shared_ptr<Chunk> adjacentChunk = world->getChunk(bottomChunkX, bottomChunkY, bottomChunkZ)) {
                            adjacentChunk->deleteBlock(blockLocalPosition[0], maxBlockPositionInAdjacentChunk, blockLocalPosition[2], type);
                            world->getMeshesToUpdate().push(adjacentChunk);
                        }
                    }
                    if (isAtTopBorder) {
                        const int topChunkX = chunk->getX();
                        const int topChunkY = chunk->getY() + chunkSize;
                        const int topChunkZ = chunk->getZ();
                        if (const std::shared_ptr<Chunk> adjacentChunk = world->getChunk(topChunkX, topChunkY, topChunkZ)) {
                            adjacentChunk->deleteBlock(blockLocalPosition[0], minBlockPositionInAdjacentChunk, blockLocalPosition[2], type);
                            world->getMeshesToUpdate().push(adjacentChunk);
                        }
                    }
                    if (isAtFrontBorder) {
                        const int frontChunkX = chunk->getX();
                        const int frontChunkY = chunk->getY();
                        const int frontChunkZ = chunk->getZ() - chunkSize;
                        if (const std::shared_ptr<Chunk> adjacentChunk = world->getChunk(frontChunkX, frontChunkY, frontChunkZ)) {
                            adjacentChunk->deleteBlock(blockLocalPosition[0], blockLocalPosition[1], maxBlockPositionInAdjacentChunk, type);
                            world->getMeshesToUpdate().push(adjacentChunk);
                        }
                    }
                    if (isAtBackBorder) {
                        const int backChunkX = chunk->getX();
                        const int backChunkY = chunk->getY();
                        const int backChunkZ = chunk->getZ() + chunkSize;
                        if (const std::shared_ptr<Chunk> adjacentChunk = world->getChunk(backChunkX, backChunkY, backChunkZ)) {
                            adjacentChunk->deleteBlock(blockLocalPosition[0], blockLocalPosition[1], minBlockPositionInAdjacentChunk, type);
                            world->getMeshesToUpdate().push(adjacentChunk);
                        }
                    }

                    // Delete in the chunk (last to reduce incorrect adjacent chunk mesh until partial mesh update is implemented)
                    chunk->deleteBlock(blockLocalPosition[0], blockLocalPosition[1], blockLocalPosition[2], type);
                    world->getMeshesToUpdate().push(chunk);
                    if (type == BlockType::SURFACE_FEATURE_BILLBOARD) world->setInstancesChanged(true);

                    const auto t2 = std::chrono::high_resolution_clock::now();
                    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
                    std::cout << "Block deleted in " << duration << " ms" << std::endl;
                });
            }
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
        s_LEFT_CLICKED = false;

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

void mouse_button_callback(GLFWwindow* window, const int button, const int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        s_LEFT_CLICKED = true;
    }
}
