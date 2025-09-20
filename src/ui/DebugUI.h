#ifndef DEBUGUI_H
#define DEBUGUI_H
#include <functional>

#include "../world/Block.h"
#define GLFW_INCLUDE_NONE
#include <memory>

#include "../render/Camera.h"
#include "../render/Renderer.h"
#include "../world/generation/TerrainGenerator.h"
#include "../world/chunk/Chunk.h"
#include "GLFW/glfw3.h"
#include "imgui/imgui.h"

class DebugUI {
public:
    DebugUI();
    explicit DebugUI(const std::shared_ptr<GLFWwindow> &window);
    ~DebugUI();

    static void init(const std::shared_ptr<GLFWwindow> &window);
    static void newFrame();
    template<typename Callback>
    static void render(const unsigned int &visibleChunks, const unsigned int &totalChunks, const unsigned int &drawCmds,
                       const Camera &camera, const Block::BlockType &selectedBlockType, bool &outGravityEnabled,
                       Callback&& renderDistanceCallback);
    static void draw();

    void processInput(const std::shared_ptr<GLFWwindow> &window, Camera &camera);

private:
    bool m_uiMode, m_tabKeyPressed;
    static TerrainGenerator::NoiseValues m_noises;
};

template<typename Callback>
void DebugUI::render(const unsigned int &visibleChunks, const unsigned int &totalChunks, const unsigned int &drawCmds,
                     const Camera &camera, const Block::BlockType &selectedBlockType, bool &outGravityEnabled,
                     Callback&& renderDistanceCallback) {
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
    if (auto renderDistanceInChunks = static_cast<int>(Renderer::s_renderDistance / Chunk::SIZE);
        ImGui::SliderInt("Render Distance (chunks)", &renderDistanceInChunks, 2, 32)) {
        Renderer::s_renderDistance = static_cast<float>(renderDistanceInChunks) * Chunk::SIZE;
        std::forward<Callback>(renderDistanceCallback)();
    }

    ImGui::Separator();

    ImGui::Text("Camera:");
    const glm::vec3 cameraPosition = camera.getPos();
    ImGui::Text("Position: (%.0f, %.0f, %.0f)", cameraPosition.x, cameraPosition.y, cameraPosition.z);
    if (ImGui::Checkbox("Gravity", &outGravityEnabled)) {
        // This lambda is empty because gravity is handled in Application class
    }
    if (camera.hasCameraChangedBlock()) m_noises = TerrainGenerator::NoiseValues(
                                            static_cast<int>(cameraPosition.x), static_cast<int>(cameraPosition.z));
    ImGui::Text("C: %.3f, E: %.3f", m_noises.continentalness, m_noises.erosion);
    ImGui::Text("T: %.3f, H: %.3f", m_noises.temperature, m_noises.humidity);
    const Biome biome = TerrainGenerator::getBiome(m_noises, static_cast<int>(cameraPosition.x),
                                                   static_cast<int>(cameraPosition.z));
    const std::string_view biomeName = TerrainGenerator::getBiomeName(biome);
    ImGui::Text("Biome: %.*s", static_cast<int>(biomeName.size()), biomeName.data());

    ImGui::Separator();

    ImGui::Text("Player:");
    const std::string_view blockName = Block::getBlockName(selectedBlockType);
    ImGui::Text("Selected block: %.*s", static_cast<int>(blockName.size()), blockName.data());
    ImGui::End();
}

#endif //DEBUGUI_H
