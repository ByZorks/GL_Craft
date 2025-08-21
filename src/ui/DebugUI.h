#ifndef DEBUGUI_H
#define DEBUGUI_H
#include <functional>

#include "../world/Block.h"
#include "GLFW/glfw3.h"

class Camera;

class DebugUI {
private:
    bool m_uiMode;
    bool m_tabKeyPressed;

public:
    DebugUI();
    explicit DebugUI(GLFWwindow *window);

    ~DebugUI();

    static void init(GLFWwindow *window);
    static void newFrame();
    static void render(const unsigned int &visibleChunks, const unsigned int &totalChunks, const unsigned int &drawCmds, const Camera &camera, const BlockType &selectedBlockType, const std::function<void()>& renderDistanceCallback);
    static void draw();
    void processInput(GLFWwindow *window, Camera &camera);
};

#endif //DEBUGUI_H
