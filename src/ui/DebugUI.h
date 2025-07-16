#ifndef DEBUGUI_H
#define DEBUGUI_H
#include "imgui.h"
#include "GLFW/glfw3.h"

class Camera;

class DebugUI {
private:
    ImGuiIO m_io;
    bool m_uiMode;
    bool m_tabKeyPressed;

public:
    explicit DebugUI(GLFWwindow* window);
    ~DebugUI();

    static void newFrame();

    static void render(unsigned int visibleChunks, unsigned int totalChunks, float& renderDistance);
    static void draw();

    void processInput(GLFWwindow* window, Camera& camera);
    [[nodiscard]] bool isUIMode() const;

};

#endif //DEBUGUI_H
