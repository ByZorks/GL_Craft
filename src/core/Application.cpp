#include "Application.h"

#include <iostream>

#include "WindowUserPointers.h"
#include "../math/Raycast.h"

bool Application::m_leftClicked = false;
float Application::m_aspectRatio = 16.0f / 9.0f;

Application::Application(const int width, const int height, const char *title) : m_camera(width, height) {
    initGLFW(width, height, title);
    initGL();
    initResources();

    glfwMaximizeWindow(m_window); // Needs to be called after glfw, gl and meshes initialization
}

Application::~Application() {
    cleanup();
}

void Application::run() {
    Renderer::init();
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(m_window)) {
        const double currentTime = glfwGetTime();
        const auto deltaTime = static_cast<float>(currentTime - lastTime);
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

    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetCursorPosCallback(m_window, Camera::mouseCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);

    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    m_windowUserPointers = {
        .camera = &m_camera,
        .postProcessingMesh = nullptr, // Doesn't exist yet, will be set in initResources
        .crosshair = nullptr
    };
    glfwSetWindowUserPointer(m_window, &m_windowUserPointers);

    glfwSwapInterval(0); // Disable VSync
}

void Application::initGL() {
    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    std::cout << glGetString(GL_VERSION) << std::endl;
}

void Application::initResources() {
    m_blockShader = new Shader("../res/shaders/block.vert", "../res/shaders/block.frag");
    m_instancesShader = new Shader("../res/shaders/instances.vert", "../res/shaders/instances.frag");
    m_waterShader = new Shader("../res/shaders/water.vert", "../res/shaders/water.frag");
    m_highlightedBlockShader = new Shader("../res/shaders/highlightBlock.vert", "../res/shaders/highlightBlock.frag");
    m_crosshairShader = new Shader("../res/shaders/crosshair.vert", "../res/shaders/crosshair.frag");
    m_postProcessingShader = new Shader("../res/shaders/postProcessing.vert", "../res/shaders/postProcessing.frag");
    m_postProcessingShader->use();
    m_postProcessingShader->setUniform1i("u_SceneTexture", 0);
    m_postProcessingShader->setUniform1i("u_DepthTexture", 1);
    m_postProcessingShader->setUniform1f("u_RenderDistance", Renderer::s_renderDistance);

    m_atlas = new Texture("../res/textures/atlas/texture_atlas.png");

    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    m_postProcessingMesh = new PostProcessingMesh(width, height);
    m_crosshairMesh = new Crosshair();
    m_highlightedBlockMesh = new HighlightedBlock();
    m_windowUserPointers.postProcessingMesh = m_postProcessingMesh;
    m_windowUserPointers.crosshair = m_crosshairMesh;

    m_world = new World();
    DebugUI::init(m_window);
    m_blockSelector.init(m_highlightedBlockShader, m_highlightedBlockMesh);
}

void Application::processInput(const float deltaTime) {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(m_window, true);

    m_debugUI.processInput(m_window, m_camera); // Tab key for ImGui

    if (m_camera.isInputEnabled()) {
        m_camera.processInput(m_window, deltaTime);
    }
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

    // Chunks generation
    if (m_camera.hasCameraChangedChunk()) m_world->updateChunks(m_camera);

    // Uniforms
    m_postProcessingShader->use();
    m_postProcessingShader->setUniform1b("u_IsUnderWater", m_camera.isUnderWater(World::getHeight(
                                           static_cast<int>(m_camera.getCameraPos().x),
                                           static_cast<int>(m_camera.getCameraPos().z))));

    m_blockShader->use();
    m_blockShader->setUniformMat4f("u_MVP", m_camera.getMVP());

    m_waterShader->use();
    m_waterShader->setUniformMat4f("u_MVP", m_camera.getMVP());
    m_waterShader->setUniform1f("u_Time", static_cast<float>(glfwGetTime()));
    m_waterShader->setUniform3f("u_CameraPos", m_camera.getCameraPos().x,
                              m_camera.getCameraPos().y,
                              m_camera.getCameraPos().z);

    m_instancesShader->use();
    m_instancesShader->setUniformMat4f("u_MVP", m_camera.getMVP());

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
    Renderer::clear();
    DebugUI::newFrame();

    // Instances, chunks, transparent, and water rendering
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
    FrameBuffer::unbind();
    m_postProcessingShader->use();
    m_postProcessingMesh->getFBO().getColorTexture().bind(0);
    m_postProcessingMesh->getFBO().getDepthTexture().bind(1);

    Renderer::disableDepthTesting();
    Renderer::draw(m_postProcessingMesh->getVAO(), m_postProcessingMesh->getIBO());

    m_atlas->bind();
    m_crosshairShader->use();
    m_crosshairShader->setUniform1f("u_AspectRatio", m_aspectRatio);
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
    delete m_blockShader;
    delete m_instancesShader;
    delete m_waterShader;
    delete m_highlightedBlockShader;
    delete m_highlightedBlockMesh;
    delete m_crosshairShader;
    delete m_crosshairMesh;
    delete m_postProcessingShader;
    delete m_postProcessingMesh;
    delete m_world;

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Application::framebufferSizeCallback(GLFWwindow *window, const int width, const int height) {
    glViewport(0, 0, width, height);
    const auto pointers = static_cast<WindowUserPointers *>(glfwGetWindowUserPointer(window));
    if (!pointers) return;

    pointers->camera->setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    pointers->postProcessingMesh->resize(width, height);

    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

void Application::mouseButtonCallback(GLFWwindow *window, const int button, const int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        m_leftClicked = true;
    }
}
