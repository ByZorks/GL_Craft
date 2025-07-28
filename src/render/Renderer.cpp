#include "Renderer.h"

#include "../core/OpenGLDebug.h"
#include "../world/Chunk.h"
#include "GL/glew.h"

float Renderer::m_deltaTime = 0.0f;
float Renderer::m_lastFrame = 0.0f;
float Renderer::m_renderDistance = 20.0f * static_cast<float>(Chunk::SIZE); // Render distance in blocks

void Renderer::init() {
    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glFrontFace(GL_CCW));
    GLCall(glEnable(GL_BLEND));
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
}

void Renderer::clear() {
    GLCall(glClearColor(0.54f, 0.82f, 0.9f, 1.0f)); // Sky blue background
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

float Renderer::calculateDeltaTime(const float currentFrame) {
    m_deltaTime = currentFrame - m_lastFrame;
    m_lastFrame = currentFrame;
    return m_deltaTime;
}

void Renderer::draw(const VertexArray& vao, const IndexBuffer& ibo) {
    vao.bind();
    ibo.bind();
    GLCall(glDrawElements(GL_TRIANGLES, ibo.m_count(), GL_UNSIGNED_INT, nullptr));
}