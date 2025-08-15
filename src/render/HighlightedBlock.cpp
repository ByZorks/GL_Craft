#include "HighlightedBlock.h"

#include "Renderer.h"

HighlightedBlock::HighlightedBlock(const int x, const int y, const int z) : m_x(x), m_y(y), m_z(z) {

}

void HighlightedBlock::createGLBuffers() {
    m_VBO.init(m_vertices.data(), m_vertices.size() * sizeof(HighlightedVertex));
    m_IBO.init(m_indices.data(), m_indices.size());

    VertexBufferLayout meshLayout;
    meshLayout.PushInt<unsigned char>(3); // x, y, z
    meshLayout.PushInt<unsigned char>(3, true); // r, g, b

    m_VAO.init();
    m_VAO.addBuffer(m_VBO, meshLayout);

    m_state = State::READY_TO_DRAW;
}

void HighlightedBlock::draw() const {
    Renderer::drawLines(m_VAO, m_IBO);
}

int HighlightedBlock::getX() const {
    return m_x;
}

int HighlightedBlock::getY() const {
    return m_y;
}

int HighlightedBlock::getZ() const {
    return m_z;
}

State HighlightedBlock::getState() const {
    return m_state;
}


