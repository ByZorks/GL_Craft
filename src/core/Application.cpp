#include "Application.h"

#include <iostream>

#include "../math/Raycast.h"
#include "../render/Renderer.h"

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
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        throw std::runtime_error("Failed to initialize OpenGL context");
    }

    std::cout << glGetString(GL_VERSION) << std::endl;
}

void Application::initResources() {
    m_textures = std::make_unique<TextureArray>(16, 16, "../res/textures/");
    m_textures->bind(m_atlasTextureSlot);

    m_MVPBuffer = std::make_unique<UniformBuffer>();
    m_MVPBuffer->init(nullptr, sizeof(glm::mat4), m_MVPUniformBufferBindingSlot);

    m_timeBuffer = std::make_unique<UniformBuffer>();
    m_timeBuffer->init(nullptr, sizeof(float), m_timeUniformBufferBindingSlot);

    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    m_postProcessingMesh = std::make_unique<PostProcessingMesh>(width, height);
    m_crosshairMesh = std::make_unique<Crosshair>();
    m_highlightedBlockMesh = std::make_unique<HighlightedBlock>();

    m_blockShader = std::make_unique<Shader>("../res/shaders/block.vert", "../res/shaders/block.frag");
    m_blockShader->use();
    m_blockShader->setUniform1i("u_TextureArray", m_atlasTextureSlot);

    m_instancesShader = std::make_unique<Shader>("../res/shaders/instances.vert", "../res/shaders/instances.frag");
    m_instancesShader->use();
    m_instancesShader->setUniform1i("u_TextureArray", m_atlasTextureSlot);

    m_waterShader = std::make_unique<Shader>("../res/shaders/water.vert", "../res/shaders/water.frag");
    m_waterShader->use();
    m_waterShader->setUniform1i("u_TextureArray", m_atlasTextureSlot);

    m_highlightedBlockShader = std::make_unique<Shader>("../res/shaders/highlightBlock.vert", "../res/shaders/highlightBlock.frag");

    m_crosshairShader = std::make_unique<Shader>("../res/shaders/crosshair.vert", "../res/shaders/crosshair.frag");
    m_crosshairShader->use();
    m_crosshairShader->setUniform1i("u_TextureArray", m_atlasTextureSlot);
    m_crosshairShader->setUniform1f("u_AspectRatio", m_aspectRatio);

    m_postProcessingShader = std::make_unique<Shader>("../res/shaders/postProcessing.vert", "../res/shaders/postProcessing.frag");
    m_postProcessingShader->use();
    m_postProcessingShader->setUniform1i("u_SceneTexture", m_postProcessingSceneTextureSlot);
    m_postProcessingShader->setUniform1i("u_DepthTexture", m_postProcessingDepthTextureSlot);
    m_postProcessingShader->setUniform1f("u_RenderDistance", Renderer::s_renderDistance);

    m_world = std::make_unique<World>();
    DebugUI::init(m_window);
}

void Application::processInput(const double deltaTime) {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(m_window, true);

    m_debugUI.processInput(m_window, m_camera); // Tab key for ImGui

    if (m_camera.isInputEnabled()) m_camera.processInput(m_window, deltaTime);
}

void Application::update() {
    m_drawCmds = 0;

    // Update camera position and view matrix
    m_camera.calculateMVP();
    m_frustum = Camera::getFrustum(m_camera.getMVP());
    m_MVPBuffer->updateData(m_camera.getMVPData(), sizeof(glm::mat4));

    // Chunks generation
    m_world->update(m_camera, m_frustum);

    // Uniforms
    const auto time = static_cast<float>(glfwGetTime());
    m_timeBuffer->updateData(&time, sizeof(float));

    if (m_camera.hasCameraChangedBlock()) {
        m_postProcessingShader->use();
        TerrainGenerator::NoiseValues noises;
        noises.computeHeightNoises(static_cast<int>(m_camera.getPos().x), static_cast<int>(m_camera.getPos().z));
        m_postProcessingShader->setUniform1b("u_IsUnderWater", m_camera.isUnderWater(TerrainGenerator::getHeight(noises)));
    }

    m_waterShader->use();
    m_waterShader->setUniform3f("u_CameraPos",
                                m_camera.getPos().x,
                                m_camera.getPos().y,
                                m_camera.getPos().z);

    // Raycasting
    if (m_raycastResult = Raycast::castRay(m_camera.getPos(), m_camera.getFront(), m_world->getWorldManagerConst().getLoadedChunks());
        m_raycastResult.hitBlock) {
        if (!m_camera.isInputEnabled()) return;

        if (m_leftClicked) {
            m_world->getWorldManager().deleteBlockAndUpdateNeighbors(m_raycastResult);
        } else if (m_rightClicked) {
            m_world->getWorldManager().placeBlockAndUpdateNeighbors(m_raycastResult, m_player.getSelectedBlockType());
        } else if (m_middleClicked) {
            m_player.setSelectedBlockType(m_raycastResult.blockType);
        }
    }
}

void Application::render() {
    // Clear the screen
    m_postProcessingMesh->getFBO().bind();
    Renderer::clear();
    DebugUI::newFrame();

    // Instances, chunks, and water rendering
    m_world->draw(*m_blockShader, *m_waterShader, *m_instancesShader, m_drawCmds);

    // Block highlighting
    if (m_raycastResult.hitBlock) {
        m_highlightedBlockShader->use();
        m_highlightedBlockShader->setUniform3f("u_Offset",
            m_raycastResult.blockWorldPosition[0],
            m_raycastResult.blockWorldPosition[1],
            m_raycastResult.blockWorldPosition[2]);
        m_highlightedBlockMesh->draw();
        ++m_drawCmds;
    }

    // Post-processing and ui
    Renderer::disableDepthTesting();
    FrameBuffer::unbind();
    m_postProcessingMesh->getFBO().getColorTexture().bind(m_postProcessingSceneTextureSlot);
    m_postProcessingMesh->getFBO().getDepthTexture().bind(m_postProcessingDepthTextureSlot);
    m_postProcessingShader->use();
    m_postProcessingMesh->draw();

    m_crosshairShader->use();
    m_crosshairMesh->draw();
    Renderer::enableDepthTesting();

    // ImGui
    DebugUI::render(m_world->getWorldRendererConst().getVisibleChunksCount(), m_world->getWorldManagerConst().getLoadedChunks().size(), m_drawCmds, m_camera, m_player.getSelectedBlockType(), [this] {
        m_world->getWorldManager().updateRenderDistance(*m_postProcessingShader, m_camera, m_world->getWorldRenderer().getIndirectRenderer());
    });
    DebugUI::draw();
}

void Application::stateUpdate() {
    m_camera.updateLastState();
    m_leftClicked = false;
    m_rightClicked = false;
    m_middleClicked = false;
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
    m_textures.reset();
    m_MVPBuffer.reset();
    m_timeBuffer.reset();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Application::onMouseMove(const double xPos, const double yPos) {
    if (m_camera.isInputEnabled()) m_camera.handleMouse(xPos, yPos);
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
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        m_rightClicked = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
        m_middleClicked = true;
    }
}
