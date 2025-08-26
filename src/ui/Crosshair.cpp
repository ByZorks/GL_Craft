#include "Crosshair.h"

#include "../render/Renderer.h"

Crosshair::Crosshair() {
    createGLBuffers();
}

void Crosshair::draw() const {
    Renderer::drawElements(m_VAO, m_IBO.getCount());
}

void Crosshair::createGLBuffers() {
    m_VBO.init(m_vertices.data(), m_vertices.size() * sizeof(CrosshairVertex));
    m_IBO.init(m_indices.data(), m_indices.size());

    VertexBufferLayout layout;
    layout.Push<float>(2); // x, y
    layout.PushInt<unsigned char>(2, true); // u, v

    m_VAO.init();
    m_VAO.addBuffer(m_VBO, m_IBO, layout);
}
