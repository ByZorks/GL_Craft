#include "Renderer.h"

#include <GLFW/glfw3.h>

#include "../world/chunk/Chunk.h"

#ifndef DEBUG_BUILD
float Renderer::s_renderDistance = 16.0f * static_cast<float>(Chunk::SIZE); // Render distance in blocks
#else
float Renderer::s_renderDistance = 8.0f * static_cast<float>(Chunk::SIZE); // Render distance in blocks
#endif

bool Renderer::s_vSync = false;
int Renderer::s_swapInterval = 60; // Default to 60 Hz, will be updated when toggling V-Sync

void Renderer::init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(2); // Not all GPUs support this, but it will still benefit most of them
}

void Renderer::clear() {
    glClearColor(0.54f, 0.82f, 0.9f, 1.0f); // Sky blue background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool * Renderer::toggleVSync() {
    // s_vSync will be updated by the UI
    if (s_vSync) {
        if (GLFWmonitor *monitor = glfwGetPrimaryMonitor()) {
            if (const GLFWvidmode *mode = glfwGetVideoMode(monitor)) {
                s_swapInterval = mode->refreshRate;
            }
        }
    } else {
        s_swapInterval = 0;
    }
    glfwSwapInterval(s_swapInterval);

    return &s_vSync;
}

void Renderer::disableWireFrameMode() {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::enableWireFrameMode() {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void Renderer::disableDepthTesting() {
    glDisable(GL_DEPTH_TEST);
}

void Renderer::enableDepthTesting() {
    glEnable(GL_DEPTH_TEST);
}

void Renderer::disableDepthMask() {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::enableDepthMask() {
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

void Renderer::disableBackFaceCulling() {
    glDisable(GL_CULL_FACE);
}

void Renderer::enableBackFaceCulling() {
    glEnable(GL_CULL_FACE);
}

void Renderer::drawLines(const VertexArray &vao, const unsigned int IBOCount) {
    vao.bind();
    glDrawElements(GL_LINES, static_cast<GLsizei>(IBOCount), GL_UNSIGNED_INT, nullptr);
}

void Renderer::drawElements(const VertexArray &vao, const unsigned int IBOCount) {
    vao.bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(IBOCount), GL_UNSIGNED_INT, nullptr);
}

void Renderer::drawWithVertexPulling(const VertexArray &vao, const StorageBuffer &ssbo,
                                     const unsigned int vertexCount) {
    vao.bind();
    ssbo.bind();
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
}

void Renderer::drawWithVertexPullingInstanced(const VertexArray &vao, const StorageBuffer &ssbo,
                                              const StorageBuffer &instanceSsbo, const unsigned int vertexCount,
                                              const unsigned int instanceCount) {
    vao.bind();
    ssbo.bind();
    instanceSsbo.bind();
    glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount), static_cast<GLsizei>(instanceCount));
}

void Renderer::drawMultiWithVertexPulling(const IndirectBuffer &cmds, const StorageBuffer &vertices,
                                          const StorageBuffer &offsets, const unsigned int drawCount,
                                          const void *offset) {
    cmds.bind();
    vertices.bind();
    offsets.bind();
    glMultiDrawArraysIndirect(GL_TRIANGLES, offset, static_cast<GLsizei>(drawCount), 0);
}

void Renderer::drawElementsInstanced(const VertexArray &vao, const unsigned int IBOCount,
                                     const unsigned int instanceCount) {
    vao.bind();
    glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(IBOCount), GL_UNSIGNED_INT, nullptr,
                            static_cast<GLsizei>(instanceCount));
}
