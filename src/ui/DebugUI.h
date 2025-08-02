#ifndef DEBUGUI_H
#define DEBUGUI_H
#include "imgui.h"
#include "GLFW/glfw3.h"

class Camera;

class DebugUI {
private:
    bool m_uiMode;
    bool m_tabKeyPressed;

public:
    explicit DebugUI(GLFWwindow* window);
    ~DebugUI();

    static void newFrame();
    static void render(unsigned int visibleChunks, unsigned int visibleVegetations, unsigned int totalChunks, unsigned int totalVegetations, const Camera &camera);
    static void draw();

    void processInput(GLFWwindow* window, Camera& camera);

};

#endif //DEBUGUI_H
