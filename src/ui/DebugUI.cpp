#include "DebugUI.h"

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

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
