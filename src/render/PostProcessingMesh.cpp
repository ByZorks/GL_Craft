#include "PostProcessingMesh.h"

PostProcessingMesh::PostProcessingMesh(const int width, const int height) : m_width(width), m_height(height), m_FBO(width, height) {
    m_VBO.init(m_vertices.data(), m_vertices.size() * sizeof(uint8_t));
    m_IBO.init(m_indices.data(), m_indices.size());

    VertexBufferLayout layout;
    layout.Push<char>(2); // x, y
    layout.Push<unsigned char>(2); // u, v

    m_VAO.init();
    m_VAO.addBuffer(m_VBO, layout);
}

void PostProcessingMesh::resize(const int width, const int height) {
    m_width = width;
    m_height = height;
    m_FBO = FrameBuffer(width, height);
}

const VertexArray & PostProcessingMesh::getVAO() const {
    return m_VAO;
}

const IndexBuffer & PostProcessingMesh::getIBO() const {
    return m_IBO;
}

const FrameBuffer & PostProcessingMesh::getFBO() const {
    return m_FBO;
}
