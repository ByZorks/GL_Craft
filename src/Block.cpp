#include "Block.h"

#include <algorithm>

Block::Block(const float x, const float y, const float z, const float index) {
    constexpr float TEXTURE_OFFSET_0 = 0.0f;
    constexpr float TEXTURE_OFFSET_33 = 1.0f / 3.0f;
    constexpr float TEXTURE_OFFSET_66 = 2.0f / 3.0f;
    constexpr float TEXTURE_OFFSET_100 = 1.0f;
    constexpr float BLOCK_MIN = 0.0f;
    constexpr float BLOCK_MAX = 1.0f;


    float tempVertices[] = {
        // x, y, z, u, v
        // Front face
        x - BLOCK_MIN, y + BLOCK_MAX, z + BLOCK_MAX, index + TEXTURE_OFFSET_33, index + TEXTURE_OFFSET_100,
        x + BLOCK_MAX, y + BLOCK_MAX, z + BLOCK_MAX, index + TEXTURE_OFFSET_66, index + TEXTURE_OFFSET_100,
        x + BLOCK_MAX, y - BLOCK_MIN, z + BLOCK_MAX, index + TEXTURE_OFFSET_66, index + TEXTURE_OFFSET_0,
        x - BLOCK_MIN, y - BLOCK_MIN, z + BLOCK_MAX, index + TEXTURE_OFFSET_33, index + TEXTURE_OFFSET_0,
        // Back face
        x - BLOCK_MIN, y + BLOCK_MAX, z - BLOCK_MIN, index + TEXTURE_OFFSET_33, index + TEXTURE_OFFSET_100,
        x + BLOCK_MAX, y + BLOCK_MAX, z - BLOCK_MIN, index + TEXTURE_OFFSET_66, index + TEXTURE_OFFSET_100,
        x + BLOCK_MAX, y - BLOCK_MIN, z - BLOCK_MIN, index + TEXTURE_OFFSET_66, index + TEXTURE_OFFSET_0,
        x - BLOCK_MIN, y - BLOCK_MIN, z - BLOCK_MIN, index + TEXTURE_OFFSET_33, index + TEXTURE_OFFSET_0,
        // Left face
        x - BLOCK_MIN, y + BLOCK_MAX, z + BLOCK_MAX, index + TEXTURE_OFFSET_33, index + TEXTURE_OFFSET_100,
        x - BLOCK_MIN, y + BLOCK_MAX, z - BLOCK_MIN, index + TEXTURE_OFFSET_66, index + TEXTURE_OFFSET_100,
        x - BLOCK_MIN, y - BLOCK_MIN, z - BLOCK_MIN, index + TEXTURE_OFFSET_66, index + TEXTURE_OFFSET_0,
        x - BLOCK_MIN, y - BLOCK_MIN, z + BLOCK_MAX, index + TEXTURE_OFFSET_33, index + TEXTURE_OFFSET_0,
        // Right face
        x + BLOCK_MAX, y + BLOCK_MAX, z + BLOCK_MAX, index + TEXTURE_OFFSET_33, index + TEXTURE_OFFSET_100,
        x + BLOCK_MAX, y + BLOCK_MAX, z - BLOCK_MIN, index + TEXTURE_OFFSET_66, index + TEXTURE_OFFSET_100,
        x + BLOCK_MAX, y - BLOCK_MIN, z - BLOCK_MIN, index + TEXTURE_OFFSET_66, index + TEXTURE_OFFSET_0,
        x + BLOCK_MAX, y - BLOCK_MIN, z + BLOCK_MAX, index + TEXTURE_OFFSET_33, index + TEXTURE_OFFSET_0,
        // Top face
        x - BLOCK_MIN, y + BLOCK_MAX, z + BLOCK_MAX, index + TEXTURE_OFFSET_66, index + TEXTURE_OFFSET_100,
        x + BLOCK_MAX, y + BLOCK_MAX, z + BLOCK_MAX, index + TEXTURE_OFFSET_100, index + TEXTURE_OFFSET_100,
        x + BLOCK_MAX, y + BLOCK_MAX, z - BLOCK_MIN, index + TEXTURE_OFFSET_100, index + TEXTURE_OFFSET_0,
        x - BLOCK_MIN, y + BLOCK_MAX, z - BLOCK_MIN, index + TEXTURE_OFFSET_66, index + TEXTURE_OFFSET_0,
        // Bottom face
        x - BLOCK_MIN, y - BLOCK_MIN, z + BLOCK_MAX, index + TEXTURE_OFFSET_0, index + TEXTURE_OFFSET_100,
        x + BLOCK_MAX, y - BLOCK_MIN, z + BLOCK_MAX, index + TEXTURE_OFFSET_33, index + TEXTURE_OFFSET_100,
        x + BLOCK_MAX, y - BLOCK_MIN, z - BLOCK_MIN, index + TEXTURE_OFFSET_33, index + TEXTURE_OFFSET_0,
        x - BLOCK_MIN, y - BLOCK_MIN, z - BLOCK_MIN, index + TEXTURE_OFFSET_0, index + TEXTURE_OFFSET_0,
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

