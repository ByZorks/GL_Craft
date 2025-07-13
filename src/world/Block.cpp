#include "Block.h"

#include <stdexcept>

Block::Block(const float x, const float y, const float z, const float index) : m_x(x), m_y(y), m_z(z), m_index(index) {
}

Block::~Block() = default;

void Block::addFace(const face face) {
    // Skip if face already added
    if (m_addedFaces.contains(face)) {
        return;
    }

    // Add face to the set of added faces
    m_addedFaces.insert(face);

    // Constants for texture coordinates and block dimensions
    constexpr float TEXTURE_OFFSET_0 = 0.0f;
    constexpr float TEXTURE_OFFSET_33 = 1.0f / 3.0f;
    constexpr float TEXTURE_OFFSET_66 = 2.0f / 3.0f;
    constexpr float TEXTURE_OFFSET_100 = 1.0f;
    constexpr float BLOCK_MIN = 0.0f;
    constexpr float BLOCK_MAX = 1.0f;

    // Generate vertices for the requested face and add them to m_vertices
    std::vector<float> faceVertices;

    switch (face) {
        case face::FRONT: {
            // Front face vertices (x, y, z, u, v)
            std::vector frontVertices = {
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z + BLOCK_MAX, m_index + TEXTURE_OFFSET_33, m_index + TEXTURE_OFFSET_100,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z + BLOCK_MAX, m_index + TEXTURE_OFFSET_66, m_index + TEXTURE_OFFSET_100,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z + BLOCK_MAX, m_index + TEXTURE_OFFSET_66, m_index + TEXTURE_OFFSET_0,
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z + BLOCK_MAX, m_index + TEXTURE_OFFSET_33, m_index + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), frontVertices.begin(), frontVertices.end());
            break;
        }
        case face::BACK: {
            // Back face vertices
            std::vector backVertices = {
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z - BLOCK_MIN, m_index + TEXTURE_OFFSET_33, m_index + TEXTURE_OFFSET_100,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z - BLOCK_MIN, m_index + TEXTURE_OFFSET_66, m_index + TEXTURE_OFFSET_100,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z - BLOCK_MIN, m_index + TEXTURE_OFFSET_66, m_index + TEXTURE_OFFSET_0,
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z - BLOCK_MIN, m_index + TEXTURE_OFFSET_33, m_index + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), backVertices.begin(), backVertices.end());
            break;
        }
        case face::LEFT: {
            // Left face vertices
            std::vector leftVertices = {
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z + BLOCK_MAX, m_index + TEXTURE_OFFSET_33, m_index + TEXTURE_OFFSET_100,
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z - BLOCK_MIN, m_index + TEXTURE_OFFSET_66, m_index + TEXTURE_OFFSET_100,
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z - BLOCK_MIN, m_index + TEXTURE_OFFSET_66, m_index + TEXTURE_OFFSET_0,
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z + BLOCK_MAX, m_index + TEXTURE_OFFSET_33, m_index + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), leftVertices.begin(), leftVertices.end());
            break;
        }
        case face::RIGHT: {
            // Right face vertices
            std::vector rightVertices = {
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z + BLOCK_MAX, m_index + TEXTURE_OFFSET_33, m_index + TEXTURE_OFFSET_100,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z - BLOCK_MIN, m_index + TEXTURE_OFFSET_66, m_index + TEXTURE_OFFSET_100,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z - BLOCK_MIN, m_index + TEXTURE_OFFSET_66, m_index + TEXTURE_OFFSET_0,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z + BLOCK_MAX, m_index + TEXTURE_OFFSET_33, m_index + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), rightVertices.begin(), rightVertices.end());
            break;
        }
        case face::TOP: {
            // Top face vertices
            std::vector topVertices = {
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z + BLOCK_MAX, m_index + TEXTURE_OFFSET_66, m_index + TEXTURE_OFFSET_100,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z + BLOCK_MAX, m_index + TEXTURE_OFFSET_100, m_index + TEXTURE_OFFSET_100,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z - BLOCK_MIN, m_index + TEXTURE_OFFSET_100, m_index + TEXTURE_OFFSET_0,
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z - BLOCK_MIN, m_index + TEXTURE_OFFSET_66, m_index + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), topVertices.begin(), topVertices.end());
            break;
        }
        case face::BOTTOM: {
            // Bottom face vertices
            std::vector bottomVertices = {
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z + BLOCK_MAX, m_index + TEXTURE_OFFSET_0, m_index + TEXTURE_OFFSET_100,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z + BLOCK_MAX, m_index + TEXTURE_OFFSET_33, m_index + TEXTURE_OFFSET_100,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z - BLOCK_MIN, m_index + TEXTURE_OFFSET_33, m_index + TEXTURE_OFFSET_0,
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z - BLOCK_MIN, m_index + TEXTURE_OFFSET_0, m_index + TEXTURE_OFFSET_0
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
    // Generate indices dynamically based on added faces
    m_indices.clear();

    for (const auto& face : m_addedFaces) {
        unsigned int baseIndex = static_cast<unsigned int>(std::distance(m_addedFaces.begin(), m_addedFaces.find(face))) * 4;

        switch (face) {
            case face::FRONT:
            case face::RIGHT:
            case face::BOTTOM:
                m_indices.push_back(baseIndex);
                m_indices.push_back(baseIndex + 1);
                m_indices.push_back(baseIndex + 2);

                m_indices.push_back(baseIndex);
                m_indices.push_back(baseIndex + 2);
                m_indices.push_back(baseIndex + 3);
                break;
            case face::BACK:
            case face::LEFT:
            case face::TOP:
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

        // Add two triangles for this face

    }

    return m_indices.data();
}
