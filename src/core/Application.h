#ifndef GL_CRAFT_APPLICATION_H
#define GL_CRAFT_APPLICATION_H
#include "WindowUserPointers.h"
#include "../gl/Shader.h"
#include "../render/BlockSelector.h"
#include "../render/Camera.h"
#include "../render/HighlightedBlock.h"
#include "../render/PostProcessingMesh.h"
#include "../ui/Crosshair.h"
#include "../ui/DebugUI.h"
#include "../world/World.h"
#include "GLFW/glfw3.h"

class Application {
private:
    GLFWwindow* m_window{};
    WindowUserPointers m_windowUserPointers{};
    Camera m_camera;
    World* m_world{};
    DebugUI m_debugUI;
    Frustum m_frustum;
    BlockSelector m_blockSelector;
    RaycastResult m_raycastResult;

    PostProcessingMesh* m_postProcessingMesh{};
    HighlightedBlock* m_highlightedBlockMesh{};
    Crosshair* m_crosshairMesh{};

    Shader* m_blockShader{};
    Shader* m_instancesShader{};
    Shader* m_waterShader{};
    Shader* m_highlightedBlockShader{};
    Shader* m_crosshairShader{};
    Shader* m_postProcessingShader{};

    Texture* m_atlas{};

    unsigned int m_drawCalls = 0;
    unsigned int m_visibleChunksCount = 0;
    static bool m_leftClicked;
    static float m_aspectRatio;

public:
    Application(int width, int height, const char *title);
    ~Application();

    void run();

private:
    void initGLFW(int width, int height, const char *title);
    static void initGL();
    void initResources();
    void processInput(double deltaTime);
    void update();
    void render();
    void stateUpdate();
    void cleanup();

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
};

#endif //GL_CRAFT_APPLICATION_H