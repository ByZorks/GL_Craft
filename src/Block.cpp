#include "Block.h"

#include <algorithm>

Block::Block(const float x, const float y, const float z, const float index) {
    float tempVertices[] = {
        // x, y, z, u, v
        // Front face
        x - .5f, y + .5f, z + .5f, index + .333f, index + 1.0f,
        x + .5f, y + .5f, z + .5f, index + .666f, index + 1.0f,
        x + .5f, y - .5f, z + .5f, index + .666f, index + .0f,
        x - .5f, y - .5f, z + .5f, index + .333f, index + .0f,
        // Back face
        x - .5f, y + .5f, z - .5f, index + .333f, index + 1.0f,
        x + .5f, y + .5f, z - .5f, index + .666f, index + 1.0f,
        x + .5f, y - .5f, z - .5f, index + .666f, index + .0f,
        x - .5f, y - .5f, z - .5f, index + .333f, index + .0f,
        // Left face
        x - .5f, y + .5f, z + .5f, index + .333f, index + 1.0f,
        x - .5f, y + .5f, z - .5f, index + .666f, index + 1.0f,
        x - .5f, y - .5f, z - .5f, index + .666f, index + .0f,
        x - .5f, y - .5f, z + .5f, index + .333f, index + .0f,
        // Right face
        x + .5f, y + .5f, z + .5f, index + .333f, index + 1.0f,
        x + .5f, y + .5f, z - .5f, index + .666f, index + 1.0f,
        x + .5f, y - .5f, z - .5f, index + .666f, index + .0f,
        x + .5f, y - .5f, z + .5f, index + .333f, index + .0f,
        // Top face
        x - .5f, y + .5f, z + .5f, index + .666f, index + 1.0f,
        x + .5f, y + .5f, z + .5f, index + 1.0f, index + 1.0f,
        x + .5f, y + .5f, z - .5f, index + 1.0f, index + .0f,
        x - .5f, y + .5f, z - .5f, index + .666f, index + .0f,
        // Bottom face
        x - .5f, y - .5f, z + .5f, index + .0f, index + 1.0f,
        x + .5f, y - .5f, z + .5f, index + .333f, index + 1.0f,
        x + .5f, y - .5f, z - .5f, index + .333f, index + .0f,
        x - .5f, y - .5f, z - .5f, index + .0f, index + .0f,
    };

    std::copy_n(tempVertices, 120, m_Vertices);

    m_VBO.init(m_Vertices, sizeof(m_Vertices));
    m_IBO.init(m_Indices, sizeof(m_Indices) / sizeof(unsigned int));

    m_layout.Push<float>(3); // x, y, z
    m_layout.Push<float>(2); // u, v
    m_VAO.AddBuffer(m_VBO, m_layout);
}

Block::~Block() = default;

const VertexArray & Block::m_vao() const {
    return m_VAO;
}

const IndexBuffer & Block::m_ibo() const {
    return m_IBO;
}

