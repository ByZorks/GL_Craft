#ifndef GL_CRAFT_APPLICATION_H
#define GL_CRAFT_APPLICATION_H
#include "../entity/Player.h"
#include "../gl/Shader.h"
#include "../gl/UniformBuffer.h"
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

    std::unique_ptr<World> m_world;
    std::unique_ptr<PostProcessingMesh> m_postProcessingMesh;
    std::unique_ptr<HighlightedBlock> m_highlightedBlockMesh;
    std::unique_ptr<Crosshair> m_crosshairMesh;

    std::unique_ptr<Shader> m_blockShader;
    std::unique_ptr<Shader> m_instancesShader;
    std::unique_ptr<Shader> m_waterShader;
    std::unique_ptr<Shader> m_highlightedBlockShader;
    std::unique_ptr<Shader> m_crosshairShader;
    std::unique_ptr<Shader> m_postProcessingShader;
    std::unique_ptr<Texture> m_atlas;
    std::unique_ptr<UniformBuffer> m_MVPBuffer;

    Camera m_camera;
    DebugUI m_debugUI;
    Frustum m_frustum;
    RaycastResult m_raycastResult;
    Player m_player;

    unsigned int m_drawCalls = 0;
    unsigned int m_visibleChunksCount = 0;
    bool m_leftClicked, m_rightClicked, m_middleClicked = false;
    float m_aspectRatio = 16.0f / 9.0f;
    int m_MVPUniformBufferBindingSlot = 0;
    int m_atlasTextureSlot = 0;
    int m_postProcessingSceneTextureSlot = 1;
    int m_postProcessingDepthTextureSlot = 2;

public:
    Application(int width, int height, const char *title);
    ~Application();

    void init();
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

    void onMouseMove(double xPos, double yPos);
    void onFrameBufferResize(int width, int height);
    void onMouseEvent(int button, int action);
};

#endif //GL_CRAFT_APPLICATION_H