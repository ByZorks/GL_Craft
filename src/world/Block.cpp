#include "Block.h"

#include <stdexcept>

Block::Block(const float x, const float y, const float z) : m_x(x), m_y(y), m_z(z) {
}

Block::~Block() = default;

void Block::setType(const BlockType type) {
    switch (type) {
        case DIRT:
            m_columnIndex = 0;
            break;
        case GRASS:
            m_columnIndex = 3;
            break;
        case STONE:
            m_columnIndex = 6;
            break;
        case WATER:
            m_columnIndex = 9;
            break;
    }
}

// TODO: Write a more efficient way to handle face addition
void Block::addFace(const face face) {
    if (m_addedFaces.contains(face)) return;

    m_addedFaces.insert(face);

    // Constants for texture coordinates and block dimensions
    constexpr float TEXTURE_WIDTH = 1.0f / 12.0f;
    constexpr float TEXTURE_OFFSET_0 = 0.0f;
    constexpr float TEXTURE_OFFSET_33 = TEXTURE_WIDTH;
    constexpr float TEXTURE_OFFSET_66 = TEXTURE_WIDTH * 2.0f;
    constexpr float TEXTURE_OFFSET_100 = TEXTURE_WIDTH * 3.0f;
    constexpr float TEXTURE_V_MAX = 1.0f;
    constexpr float BLOCK_MIN = 0.0f;
    constexpr float BLOCK_MAX = 1.0f;

    // Calculate base texture coordinates
    const float u_base = m_columnIndex * TEXTURE_WIDTH;
    constexpr float v_base = 0.0f; // Blocks are on a single texture row

    // Generate vertices for the requested face and add them to m_vertices
    std::vector<float> faceVertices;

    switch (face) {
        case FRONT: {
            // Front face vertices (x, y, z, u, v)
            std::vector frontVertices = {
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_OFFSET_0,
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), frontVertices.begin(), frontVertices.end());
            break;
        }
        case BACK: {
            // Back face vertices
            std::vector backVertices = {
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_OFFSET_0,
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), backVertices.begin(), backVertices.end());
            break;
        }
        case LEFT: {
            // Left face vertices
            std::vector leftVertices = {
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_V_MAX,
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_V_MAX,
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_OFFSET_0,
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), leftVertices.begin(), leftVertices.end());
            break;
        }
        case RIGHT: {
            // Right face vertices
            std::vector rightVertices = {
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_OFFSET_0,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), rightVertices.begin(), rightVertices.end());
            break;
        }
        case TOP: {
            // Top face vertices
            std::vector topVertices = {
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_100, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_100, v_base + TEXTURE_OFFSET_0,
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), topVertices.begin(), topVertices.end());
            break;
        }
        case BOTTOM: {
            // Bottom face vertices
            std::vector bottomVertices = {
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_0, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_OFFSET_0,
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_0, v_base + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), bottomVertices.begin(), bottomVertices.end());
            break;
        }
        default:
            throw std::invalid_argument("Invalid face type");
    }
}

const float* Block::getVertices() const {
    return m_vertices.data();
}

unsigned int Block::getVertexCount() const {
    return static_cast<unsigned int>(m_vertices.size()) / 5; // 5 elements per vertex
}

unsigned int Block::getIndexCount() const {
    return static_cast<unsigned int>(m_addedFaces.size() * 6); // 6 indices per face (2 triangles)
}

const unsigned int* Block::getIndices() {
    m_indices.clear();

    for (const auto& face : m_addedFaces) {
        unsigned int baseIndex = static_cast<unsigned int>(std::distance(m_addedFaces.begin(), m_addedFaces.find(face))) * 4;

        switch (face) {
            case FRONT:
            case RIGHT:
            case BOTTOM:
                m_indices.push_back(baseIndex);
                m_indices.push_back(baseIndex + 1);
                m_indices.push_back(baseIndex + 2);

                m_indices.push_back(baseIndex);
                m_indices.push_back(baseIndex + 2);
                m_indices.push_back(baseIndex + 3);
                break;
            case BACK:
            case LEFT:
            case TOP:
                m_indices.push_back(baseIndex);
                m_indices.push_back(baseIndex + 3);
                m_indices.push_back(baseIndex + 2);

                m_indices.push_back(baseIndex);
                m_indices.push_back(baseIndex + 2);
                m_indices.push_back(baseIndex + 1);
                break;
            default:
                throw std::invalid_argument("Invalid face type");
        }
    }

    return m_indices.data();
}
