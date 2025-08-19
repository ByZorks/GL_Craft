#include "InstanceRenderer.h"

#include "Renderer.h"

void InstanceRenderer::addInstance(const std::array<int, 4> &position) {
    if (m_instanceCount >= m_instancePositions.size()) {
        m_instancePositions.push_back(position);
    } else {
        m_instancePositions[m_instanceCount] = position;
    }
    m_instanceCount++;
}

void InstanceRenderer::updateInstanceBuffer() {
    if (m_instanceCount == 0 || !m_buffersInitialized) return;

    // Resize the instance buffer if necessary
    if (m_instanceCount > m_instanceBufferCapacity) {
        m_instanceBufferCapacity = m_instanceCount * 2;

        m_instanceSSBO.deleteBuffer();
        m_instanceSSBO.init(m_instancePositions.data(), m_instanceBufferCapacity * sizeof(std::array<int, 4>), 2);
    } else {
        m_instanceSSBO.updateData(m_instancePositions.data(), m_instanceCount * sizeof(std::array<int, 4>));
    }
}

void InstanceRenderer::resetInstances() {
    m_instanceCount = 0;
    m_instancePositions.clear();
}

void InstanceRenderer::draw() const {
    if (m_instanceCount == 0 || !m_buffersInitialized) return;

    Renderer::drawWithVertexPullingInstanced(m_VAO, m_SSBO, m_instanceSSBO,  m_vertices.size() * 6, m_instanceCount);
}

unsigned int InstanceRenderer::getInstancesCount() const {
    return m_instanceCount;
}
