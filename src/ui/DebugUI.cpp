#include "GL/glew.h"
#include "DebugUI.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "../world/Chunk.h"
#include "../render/Camera.h"

DebugUI::DebugUI(GLFWwindow *window): m_uiMode(false), m_tabKeyPressed(false) {
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx);

    m_io = ImGui::GetIO();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ImGui::StyleColorsDark();
}

DebugUI::~DebugUI() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void DebugUI::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void DebugUI::render(const unsigned int visibleChunks, unsigned int visibleVegetations, const unsigned int totalChunks, unsigned int totalVegetations, float &renderDistance, const Camera &camera) {
    ImGui::Begin("Debug");
    ImGui::Text("Performance:");
    const ImGuiIO& io = ImGui::GetIO(); // Cannot use m_io here because it is not updated in this function
    ImGui::Text("Application average %.3f ms/frame (%.0f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Separator();
    ImGui::Text("World:");
    ImGui::Text("Rendering: %u/%u chunks", visibleChunks, totalChunks);
    ImGui::Text("Rendering: %u/%u vegetations", visibleVegetations, totalVegetations);
    constexpr auto chunkSize = static_cast<float>(Chunk::SIZE);
    int renderDistanceInChunks = static_cast<int>(renderDistance / chunkSize);
    if (ImGui::SliderInt("Render Distance (chunks)", &renderDistanceInChunks, 1, 32)) {
        renderDistance = static_cast<float>(renderDistanceInChunks) * chunkSize;
    }
    ImGui::Separator();
    ImGui::Text("Camera:");
    ImGui::Text("Position: (%.2f, %.2f, %.2f)", camera.m_camera_pos().x, camera.m_camera_pos().y, camera.m_camera_pos().z);
    ImGui::End();
}

void DebugUI::draw() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DebugUI::processInput(GLFWwindow *window, Camera &camera) {
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !m_tabKeyPressed) {
        m_uiMode = !m_uiMode;
        glfwSetInputMode(window, GLFW_CURSOR, m_uiMode ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        camera.set_m_input_enabled(!m_uiMode);

        if (!m_uiMode) {
            camera.resetMousePosition(window);
        }
        m_tabKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE) {
        m_tabKeyPressed = false;
    }
}

bool DebugUI::isUIMode() const {
    return m_uiMode;
}
