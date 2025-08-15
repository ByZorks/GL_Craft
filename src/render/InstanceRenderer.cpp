#include "InstanceRenderer.h"

#include "Renderer.h"

void InstanceRenderer::addInstance(const std::array<int, 3> &position) {
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

        m_instanceVBO.deleteBuffer();
        m_instanceVBO.init(m_instancePositions.data(), m_instanceBufferCapacity * sizeof(std::array<int, 3>), BufferUsage::DYNAMIC);

        m_VAO.addInstancedBuffer(m_instanceVBO, 4, 3);
    } else {
        m_instanceVBO.updateData(m_instancePositions.data(), m_instanceCount * sizeof(std::array<int, 3>));
    }
}

void InstanceRenderer::resetInstances() {
    m_instanceCount = 0;
    m_instancePositions.clear();
}

void InstanceRenderer::draw() const {
    if (m_instanceCount == 0 || !m_buffersInitialized) return;

    Renderer::drawInstanced(m_VAO, m_IBO, m_instanceCount);
}

unsigned int InstanceRenderer::getInstancesCount() const {
    return m_instanceCount;
}

std::vector<std::array<int, 3>> & InstanceRenderer::getInstancesPositions() {
    return m_instancePositions;
}
