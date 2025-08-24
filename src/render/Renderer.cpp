#include "Renderer.h"

#include "../gl/OpenGLDebug.h"
#include "../world/Chunk.h"
#include "GL/glew.h"

float Renderer::m_deltaTime = 0.0f;
float Renderer::m_lastFrame = 0.0f;
float Renderer::s_renderDistance = 12.0f * static_cast<float>(Chunk::SIZE); // Render distance in blocks

void Renderer::init() {
    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glFrontFace(GL_CCW));
    GLCall(glEnable(GL_LINE_SMOOTH));
    GLCall(glLineWidth(2)); // Not all GPUs support this, but it will still benefit most of them
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

void Renderer::disableWireFrameMode() {
    GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
}

void Renderer::enableWireFrameMode() {
    GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
}

void Renderer::disableDepthTesting() {
    GLCall(glDisable(GL_DEPTH_TEST));
}

void Renderer::enableDepthTesting() {
    GLCall(glEnable(GL_DEPTH_TEST));
}

void Renderer::disableDepthMask() {
    GLCall(glEnable(GL_BLEND));
    GLCall(glDepthMask(GL_FALSE));
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
}

void Renderer::enableDepthMask() {
    GLCall(glDisable(GL_BLEND));
    GLCall(glDepthMask(GL_TRUE));
}

void Renderer::disableBackFaceCulling() {
    GLCall(glDisable(GL_CULL_FACE));
}

void Renderer::enableBackFaceCulling() {
    GLCall(glEnable(GL_CULL_FACE));
}

void Renderer::drawLines(const VertexArray &vao, const IndexBuffer &ibo) {
    vao.bind();
    ibo.bind();
    GLCall(glDrawElements(GL_LINES, ibo.getCount(), GL_UNSIGNED_INT, nullptr));
}

void Renderer::drawElements(const VertexArray& vao, const IndexBuffer& ibo) {
    vao.bind();
    ibo.bind();
    GLCall(glDrawElements(GL_TRIANGLES, ibo.getCount(), GL_UNSIGNED_INT, nullptr));
}

void Renderer::drawWithVertexPulling(const VertexArray &vao, const StorageBuffer &ssbo, const unsigned int vertexCount) {
    vao.bind();
    ssbo.bind();
    GLCall(glDrawArrays(GL_TRIANGLES, 0, vertexCount));
}

void Renderer::drawWithVertexPullingInstanced(const VertexArray &vao, const StorageBuffer &ssbo, const StorageBuffer &instanceSsbo, const unsigned int vertexCount,
    const unsigned int instanceCount) {
    vao.bind();
    ssbo.bind();
    instanceSsbo.bind();
    GLCall(glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, instanceCount));
}

void Renderer::drawMultiWithVertexPulling(const IndirectBuffer &cmds, const StorageBuffer &vertices,
                                          const StorageBuffer &offsets, const unsigned int drawCount, const void *offset) {
    cmds.bind();
    vertices.bind();
    offsets.bind();
    GLCall(glMultiDrawArraysIndirect(GL_TRIANGLES, offset, drawCount, 0));
}

void Renderer::drawElementsInstanced(const VertexArray &vao, const IndexBuffer &ibo, const unsigned int instanceCount) {
    vao.bind();
    ibo.bind();
    GLCall(glDrawElementsInstanced(GL_TRIANGLES, ibo.getCount(), GL_UNSIGNED_INT, nullptr,instanceCount));
}
