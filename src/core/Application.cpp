#include "Application.h"

#include <iostream>

#include "../math/Raycast.h"

Application::Application(const int width, const int height, const char *title) : m_camera(width, height) {
    initGLFW(width, height, title);
}

Application::~Application() {
    cleanup();
}

void Application::init() {
    initGL();
    initResources();

    glfwMaximizeWindow(m_window); // Needs to be called after glfw and meshes initialization
}

void Application::run() {
    Renderer::init();
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(m_window)) {
        const double currentTime = glfwGetTime();
        const double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        processInput(deltaTime);
        update();
        render();
        stateUpdate();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void Application::initGLFW(const int width, const int height, const char *title) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);

    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, const int w, const int h) {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
        app->onFrameBufferResize(w, h);
    });
    glfwSetCursorPosCallback(m_window, [](GLFWwindow *window, const double xpos, const double ypos) {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
        app->onMouseMove(xpos, ypos);
    });
    glfwSetMouseButtonCallback(m_window, [](GLFWwindow *window, const int button, const int action, const int mods) {
        auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
        app->onMouseEvent(button, action);
    });

    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(m_window, this);

    glfwSwapInterval(0); // Disable VSync
}

void Application::initGL() {
    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    std::cout << glGetString(GL_VERSION) << std::endl;
}

void Application::initResources() {
    m_atlas = std::make_unique<Texture>("../res/textures/atlas/atlas.png");
    m_MVPBuffer = std::make_unique<UniformBuffer>();
    m_MVPBuffer->init(nullptr, sizeof(glm::mat4), m_MVPUniformBufferBindingSlot);

    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    m_postProcessingMesh = std::make_unique<PostProcessingMesh>(width, height);
    m_crosshairMesh = std::make_unique<Crosshair>();
    m_highlightedBlockMesh = std::make_unique<HighlightedBlock>();

    m_blockShader = std::make_unique<Shader>("../res/shaders/block.vert", "../res/shaders/block.frag");
    m_blockShader->use();
    m_blockShader->setUniform1i("u_Texture", m_atlasTextureSlot);

    m_instancesShader = std::make_unique<Shader>("../res/shaders/instances.vert", "../res/shaders/instances.frag");
    m_instancesShader->use();
    m_instancesShader->setUniform1i("u_Texture", m_atlasTextureSlot);

    m_waterShader = std::make_unique<Shader>("../res/shaders/water.vert", "../res/shaders/water.frag");
    m_waterShader->use();
    m_waterShader->setUniform1i("u_Texture", m_atlasTextureSlot);

    m_highlightedBlockShader = std::make_unique<Shader>("../res/shaders/highlightBlock.vert", "../res/shaders/highlightBlock.frag");

    m_crosshairShader = std::make_unique<Shader>("../res/shaders/crosshair.vert", "../res/shaders/crosshair.frag");
    m_crosshairShader->use();
    m_crosshairShader->setUniform1i("u_Texture", m_atlasTextureSlot);
    m_crosshairShader->setUniform1f("u_AspectRatio", m_aspectRatio);

    m_postProcessingShader = std::make_unique<Shader>("../res/shaders/postProcessing.vert", "../res/shaders/postProcessing.frag");
    m_postProcessingShader->use();
    m_postProcessingShader->setUniform1i("u_SceneTexture", m_postProcessingSceneTextureSlot);
    m_postProcessingShader->setUniform1i("u_DepthTexture", m_postProcessingDepthTextureSlot);
    m_postProcessingShader->setUniform1f("u_RenderDistance", Renderer::s_renderDistance);

    m_world = std::make_unique<World>();
    DebugUI::init(m_window);
    m_blockSelector.init(m_highlightedBlockShader.get(), m_highlightedBlockMesh.get());
}

void Application::processInput(const double deltaTime) {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(m_window, true);

    m_debugUI.processInput(m_window, m_camera); // Tab key for ImGui

    if (m_camera.isInputEnabled()) m_camera.processInput(m_window, deltaTime);
}

void Application::update() {
    if (!m_world) {
        throw std::runtime_error("World not initialized");
    }

    m_drawCalls = 0;
    m_visibleChunksCount = 0;

    // Update camera position and view matrix
    m_camera.calculateMVP();
    m_frustum = Camera::getFrustum(m_camera.getMVP());
    m_MVPBuffer->updateData(m_camera.getMVPData(), sizeof(glm::mat4));

    // Chunks generation
    if (m_camera.hasCameraChangedChunk()) m_world->updateChunks(m_camera);

    // Uniforms
    m_postProcessingShader->use();
    m_postProcessingShader->setUniform1b("u_IsUnderWater", m_camera.isUnderWater(World::getHeight(
                                           static_cast<int>(m_camera.getCameraPos().x),
                                           static_cast<int>(m_camera.getCameraPos().z))));

    m_waterShader->use();
    m_waterShader->setUniform1f("u_Time", static_cast<float>(glfwGetTime()));
    m_waterShader->setUniform3f("u_CameraPos", m_camera.getCameraPos().x,
                              m_camera.getCameraPos().y,
                              m_camera.getCameraPos().z);

    // Raycasting
    if (m_raycastResult = Raycast::castRay(m_camera.getCameraPos(), m_camera.getCameraFront(), m_world->getLoadedChunks());
        m_raycastResult.hitBlock) {
        if (m_leftClicked && m_camera.isInputEnabled()) {
            m_world->deleteBlockAndUpdateNeighbors(m_raycastResult);
        }
    }
}

void Application::render() {
    // Clear the screen
    m_postProcessingMesh->getFBO().bind();
    m_atlas->bind(m_atlasTextureSlot);
    Renderer::clear();
    DebugUI::newFrame();

    // Instances, chunks, transparent, and water rendering
    m_instancesShader->use();
    m_world->drawInstances(m_drawCalls);
    m_blockShader->use();
    m_world->drawChunks(m_camera, m_frustum, *m_blockShader, m_visibleChunksCount, m_drawCalls);
    m_world->drawTransparentChunks(m_frustum, *m_blockShader, m_drawCalls);
    m_waterShader->use();
    m_world->drawWater(m_frustum, *m_waterShader, m_drawCalls);

    // Block highlighting
    if (m_raycastResult.hitBlock) {
        m_blockSelector.render(m_raycastResult,m_camera.getMVP());
        m_highlightedBlockMesh->draw();
        ++m_drawCalls;
    }

    // Post-processing and crosshair to minimize openGl state changes
    Renderer::disableDepthTesting();
    FrameBuffer::unbind();
    m_postProcessingMesh->getFBO().getColorTexture().bind(m_postProcessingSceneTextureSlot);
    m_postProcessingMesh->getFBO().getDepthTexture().bind(m_postProcessingDepthTextureSlot);
    m_postProcessingShader->use();
    m_postProcessingMesh->draw();

    m_atlas->bind(m_atlasTextureSlot);
    m_crosshairShader->use();
    m_crosshairMesh->draw();
    Renderer::enableDepthTesting();

    // ImGui
    DebugUI::render(m_visibleChunksCount, m_world->getLoadedChunks().size(), m_drawCalls, m_camera, [this] {
        m_world->updateRenderDistance(*m_postProcessingShader, m_camera);
    });
    DebugUI::draw();
}

void Application::stateUpdate() {
    m_camera.updateLastState();
    m_leftClicked = false;
}

void Application::cleanup() {
    m_raycastResult = RaycastResult{};
    Raycast::clearCache();
    m_world.reset();
    m_postProcessingMesh.reset();
    m_highlightedBlockMesh.reset();
    m_crosshairMesh.reset();
    m_blockShader.reset();
    m_instancesShader.reset();
    m_waterShader.reset();
    m_highlightedBlockShader.reset();
    m_crosshairShader.reset();
    m_postProcessingShader.reset();
    m_atlas.reset();
    m_MVPBuffer.reset();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Application::onMouseMove(const double xpos, const double ypos) {
    if (m_camera.isInputEnabled()) m_camera.handleMouse(xpos, ypos);
}

void Application::onFrameBufferResize(const int width, const int height) {
    glViewport(0, 0, width, height);
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    m_camera.setAspectRatio(m_aspectRatio);
    if (m_postProcessingMesh) m_postProcessingMesh->resize(width, height);
    if (m_crosshairShader) {
        m_crosshairShader->use();
        m_crosshairShader->setUniform1f("u_AspectRatio", m_aspectRatio);
    }
}

void Application::onMouseEvent(const int button, const int action) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        m_leftClicked = true;
    }
}
