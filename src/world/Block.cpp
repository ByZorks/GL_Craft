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
}

Block::~Block() = default;

const unsigned int * Block::getIndices() {
    static const unsigned int indices[36] = {
        // Front face
        0, 1, 2,
        0, 2, 3,
        // Back face
        4, 7, 6,
        4, 6, 5,
        // Left face
        8, 11, 10,
        8, 10, 9,
        // Right face
        12, 13, 14,
        12, 14, 15,
        // Top face
        16, 19, 18,
        16, 18, 17,
        // Bottom face
        20, 21, 22,
        20, 22, 23
    };
    return indices;
}

const float * Block::getVertices() const {
    return m_Vertices;
}
