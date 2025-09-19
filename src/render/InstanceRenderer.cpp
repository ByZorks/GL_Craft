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
    if (m_instanceCount == 0) return;

    if (const size_t newSize = m_instanceSSBO.updateData(m_instancePositions.data(),
                                                         m_instanceCount * sizeof(std::array<int, 3>));
        newSize > 0) {
        m_instancePositions.resize(newSize / sizeof(std::array<int, 3>));
    }
}

void InstanceRenderer::resetInstances() {
    m_instanceCount = 0;
    m_instancePositions.clear();
}

void InstanceRenderer::draw() const {
    if (m_instanceCount == 0) return;

    Renderer::drawWithVertexPullingInstanced(m_VAO, m_verticesSSBO, m_instanceSSBO, static_cast<unsigned int>(m_vertices.size()) * 6u,
                                             m_instanceCount);
}

unsigned int InstanceRenderer::getInstancesCount() const {
    return m_instanceCount;
}
