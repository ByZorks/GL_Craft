#ifndef GL_CRAFT_APPLICATION_H
#define GL_CRAFT_APPLICATION_H
#include "../entity/Player.h"
#include "../gl/Shader.h"
#include "../gl/TextureArray.h"
#include "../gl/UniformBuffer.h"
#include "../render/Camera.h"
#include "../render/HighlightedBlock.h"
#include "../render/PostProcessingMesh.h"
#include "../ui/Crosshair.h"
#include "../ui/DebugUI.h"
#include "../world/World.h"
#include "glad/gl.h"
#include "GLFW/glfw3.h"

class Application {
public:
    Application(int width, int height, const char *title);

    void run();

private:
    void initGLFW(int width, int height, const char *title);
    static void initGL();
    void initResources();
    void processInput(double deltaTime);
    void update();
    void render();
    void resetStates();

    void onMouseMove(double xPos, double yPos);
    void onFrameBufferResize(int width, int height);
    void onMouseEvent(int button, int action);
    void onKeyEvent(int key, int action);

    static void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity,
                                       GLsizei length, const char *message, const void *userParam);

private:
    std::shared_ptr<GLFWwindow> m_window;

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
    std::unique_ptr<TextureArray> m_textures;
    std::unique_ptr<UniformBuffer> m_MVPBuffer;
    std::unique_ptr<UniformBuffer> m_timeBuffer;

    Camera m_camera;
    DebugUI m_debugUI;
    Frustum m_frustum;
    RaycastResult m_raycastResult;
    Player m_player;

    unsigned int m_drawCmds = 0;
    bool m_leftClicked = false, m_rightClicked = false, m_middleClicked = false;
    bool m_key1Pressed = false, m_key2Pressed = false, m_key3Pressed = false;
    bool m_key4Pressed = false, m_key5Pressed = false, m_key6Pressed = false;
    bool m_key7Pressed = false, m_key8Pressed = false, m_key9Pressed = false;
    float m_aspectRatio = 16.0f / 9.0f;
    int m_MVPUniformBufferBindingSlot = 0;
    int m_timeUniformBufferBindingSlot = 1;
    int m_textureSlot = 0;
    int m_postProcessingSceneTextureSlot = 1;
    int m_postProcessingDepthTextureSlot = 2;

};

#endif //GL_CRAFT_APPLICATION_H
