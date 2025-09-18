#include "DebugUI.h"

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "../render/Camera.h"
#include "../render/Renderer.h"
#include "../world/TerrainGenerator.h"

TerrainGenerator::NoiseValues DebugUI::m_noises;

DebugUI::DebugUI() : m_uiMode(false), m_tabKeyPressed(false) {
}

DebugUI::DebugUI(const std::shared_ptr<GLFWwindow> &window) : m_uiMode(false), m_tabKeyPressed(false) {
    init(window);
}

DebugUI::~DebugUI() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void DebugUI::init(const std::shared_ptr<GLFWwindow> &window) {
    ImGuiContext *ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx);

    ImGui_ImplGlfw_InitForOpenGL(window.get(), true);
    ImGui_ImplOpenGL3_Init("#version 460 core");
    ImGui::StyleColorsDark();
}

void DebugUI::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void DebugUI::render(const unsigned int &visibleChunks, const unsigned int &totalChunks, const unsigned int &drawCmds,
                     const Camera &camera, const Block::BlockType &selectedBlockType,
                     const std::function<void()> &renderDistanceCallback) {
    ImGui::Begin("Debug");
    ImGui::Text("Performance:");
    const ImGuiIO &io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.0f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Text("Draw commands: %u", drawCmds);
    if (ImGui::Checkbox("V-Sync", &Renderer::s_vSync)) {
        Renderer::toggleVSync();
    }

    ImGui::Separator();

    ImGui::Text("World:");
    ImGui::Text("Rendering: %u/%u chunks", visibleChunks, totalChunks);
    int renderDistanceInChunks = static_cast<int>(Renderer::s_renderDistance / Chunk::SIZE);
    if (ImGui::SliderInt("Render Distance (chunks)", &renderDistanceInChunks, 2, 32)) {
        Renderer::s_renderDistance = static_cast<float>(renderDistanceInChunks) * Chunk::SIZE;
        renderDistanceCallback();
    }

    ImGui::Separator();

    ImGui::Text("Camera:");
    const glm::vec3 cameraPosition = camera.getPos();
    ImGui::Text("Position: (%.0f, %.0f, %.0f)", cameraPosition.x, cameraPosition.y, cameraPosition.z);
    if (camera.hasCameraChangedBlock()) m_noises = TerrainGenerator::NoiseValues(
                                            static_cast<int>(cameraPosition.x), static_cast<int>(cameraPosition.z));
    ImGui::Text("C: %.3f, E: %.3f", m_noises.continentalness, m_noises.erosion);
    ImGui::Text("T: %.3f, H: %.3f", m_noises.temperature, m_noises.humidity);
    const Biome biome = TerrainGenerator::getBiome(m_noises, static_cast<int>(cameraPosition.x),
                                                   static_cast<int>(cameraPosition.z));
    ImGui::Text("Biome: %s", TerrainGenerator::getBiomeName(biome));

    ImGui::Separator();

    ImGui::Text("Player:");
    ImGui::Text("Selected Block Type: %s", Block::getBlockName(selectedBlockType));
    ImGui::End();
}

void DebugUI::draw() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DebugUI::processInput(const std::shared_ptr<GLFWwindow> &window, Camera &camera) {
    if (glfwGetKey(window.get(), GLFW_KEY_TAB) == GLFW_PRESS && !m_tabKeyPressed) {
        m_uiMode = !m_uiMode;
        glfwSetInputMode(window.get(), GLFW_CURSOR, m_uiMode ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        camera.setInput(!m_uiMode);

        if (!m_uiMode) {
            camera.resetMousePosition(window.get());
        }
        m_tabKeyPressed = true;
    }
    if (glfwGetKey(window.get(), GLFW_KEY_TAB) == GLFW_RELEASE) {
        m_tabKeyPressed = false;
    }
}
