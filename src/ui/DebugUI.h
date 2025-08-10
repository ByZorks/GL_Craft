#ifndef DEBUGUI_H
#define DEBUGUI_H
#include "GLFW/glfw3.h"

class Camera;

class DebugUI {
private:
    bool m_uiMode;
    bool m_tabKeyPressed;

public:
    explicit DebugUI(GLFWwindow *window);

    ~DebugUI();

    static void newFrame();

    static void render(const unsigned int &visibleChunks, const unsigned int &totalChunks, const unsigned int &drawCalls, const Camera &camera);

    static void draw();

    void processInput(GLFWwindow *window, Camera &camera);
};

#endif //DEBUGUI_H
