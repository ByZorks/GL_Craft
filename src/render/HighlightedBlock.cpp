#include "HighlightedBlock.h"

#include "Renderer.h"

HighlightedBlock::HighlightedBlock() {
    createGLBuffers();
}

void HighlightedBlock::draw() const {
    Renderer::drawLines(m_VAO, m_IBO);
}

void HighlightedBlock::createGLBuffers() {
    m_VBO.init(m_vertices.data(), m_vertices.size() * sizeof(HighlightedVertex));
    m_IBO.init(m_indices.data(), m_indices.size());

    VertexBufferLayout meshLayout;
    meshLayout.PushInt<unsigned char>(3); // x, y, z
    meshLayout.PushInt<unsigned char>(3, true); // r, g, b

    m_VAO.init();
    m_VAO.addBuffer(m_VBO, meshLayout);
}
