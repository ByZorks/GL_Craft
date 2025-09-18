#ifndef DEBUGUI_H
#define DEBUGUI_H
#include <functional>

#include "../world/Block.h"
#define GLFW_INCLUDE_NONE
#include <memory>

#include "../world/TerrainGenerator.h"
#include "GLFW/glfw3.h"

class Camera;

class DebugUI {
public:
    DebugUI();
    explicit DebugUI(const std::shared_ptr<GLFWwindow> &window);
    ~DebugUI();

    static void init(const std::shared_ptr<GLFWwindow> &window);
    static void newFrame();
    static void render(const unsigned int &visibleChunks, const unsigned int &totalChunks, const unsigned int &drawCmds,
                       const Camera &camera, const Block::BlockType &selectedBlockType, bool &outGravityEnabled,
                       const std::function<void()> &renderDistanceCallback);
    static void draw();

    void processInput(const std::shared_ptr<GLFWwindow> &window, Camera &camera);

private:
    bool m_uiMode, m_tabKeyPressed;
    static TerrainGenerator::NoiseValues m_noises;
};

#endif //DEBUGUI_H
