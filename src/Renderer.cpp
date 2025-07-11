#include "Renderer.h"

#include "OpenGLDebug.h"
#include "GL/glew.h"

float Renderer::m_deltaTime = 0.0f;
float Renderer::m_lastFrame = 0.0f;

void Renderer::init() {
    GLCall(glEnable(GL_DEPTH_TEST));
}

void Renderer::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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