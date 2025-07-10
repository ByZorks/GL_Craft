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
        2, 3, 0,
        // Back face
        4, 5, 6,
        6, 7, 4,
        // Left face
        8, 9, 10,
        10, 11, 8,
        // Right face
        12, 13, 14,
        14, 15, 12,
        // Top face
        16, 17, 18,
        18, 19, 16,
        // Bottom face
        20, 21, 22,
        22, 23, 20
    };
    return indices;
}

const float * Block::getVertices() const {
    return m_Vertices;
}
