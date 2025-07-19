#include "Block.h"

#include <stdexcept>

#include "Chunk.h"

Block::Block(const float x, const float y, const float z) : m_x(x), m_y(y), m_z(z), m_columnIndex(0) {
    m_addedFaces.reserve(6); // Reserve space for 6 faces
}

Block::~Block() = default;

void Block::setType(const BlockType type) {
    switch (type) {
        case BlockType::BEDROCK:
            m_columnIndex = 0;
            break;
        case BlockType::DIRT:
            m_columnIndex = 3;
            break;
        case BlockType::GRASS:
            m_columnIndex = 6;
            break;
        case BlockType::STONE:
            m_columnIndex = 9;
            break;
        case BlockType::WATER:
            m_columnIndex = 12;
            break;
        default:
            throw std::runtime_error("Block::setType: invalid block type");
    }
}

// TODO: Write a more efficient way to handle face addition
void Block::addFace(const Face face) {
    if (m_addedFaces.contains(face)) return;

    m_vertices.reserve(m_vertices.size() + 6*4*5); // Reserve space for 6 faces, 4 vertices each, 5 components per vertex
    m_addedFaces.insert(face);

    // Constants for texture coordinates and block dimensions
    constexpr float TEXTURE_WIDTH = 1.0f / 15.0f;
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
        case Face::FRONT: {
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
        case Face::BACK: {
            std::vector backVertices = {
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_OFFSET_0,
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), backVertices.begin(), backVertices.end());
            break;
        }
        case Face::LEFT: {
            std::vector leftVertices = {
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_V_MAX,
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_V_MAX,
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_OFFSET_0,
                m_x - BLOCK_MIN, m_y - BLOCK_MIN, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), leftVertices.begin(), leftVertices.end());
            break;
        }
        case Face::RIGHT: {
            std::vector rightVertices = {
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_OFFSET_0,
                m_x + BLOCK_MAX, m_y - BLOCK_MIN, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_33, v_base + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), rightVertices.begin(), rightVertices.end());
            break;
        }
        case Face::TOP: {
            std::vector topVertices = {
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z + BLOCK_MAX, u_base + TEXTURE_OFFSET_100, v_base + TEXTURE_V_MAX,
                m_x + BLOCK_MAX, m_y + BLOCK_MAX, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_100, v_base + TEXTURE_OFFSET_0,
                m_x - BLOCK_MIN, m_y + BLOCK_MAX, m_z - BLOCK_MIN, u_base + TEXTURE_OFFSET_66, v_base + TEXTURE_OFFSET_0
            };
            m_vertices.insert(m_vertices.end(), topVertices.begin(), topVertices.end());
            break;
        }
        case Face::BOTTOM: {
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

    m_vertices.shrink_to_fit();
}

std::vector<float> Block::addFaceVertices(const Face face, BlockType type, const float worldX, const float worldY, const float worldZ, const float u_base) {
    constexpr float c_texture_width = 1.0f / 15.0f;
    constexpr float c_texture_offset_0 = 0.0f;
    constexpr float c_texture_offset_33 = c_texture_width;
    constexpr float c_texture_offset_66 = c_texture_width * 2.0f;
    constexpr float c_texture_offset_100 = c_texture_width * 3.0f;
    constexpr float c_texture_v_max = 1.0f;
    constexpr float c_block_min = 0.0f;
    float c_block_max = 1.0f;
    constexpr float v_base = 0.0f; // Blocks are on a single texture row

    std::vector<float> vertices;
    vertices.reserve(4 * 5); // Reserve space for 4 vertices, 5 components per vertex
    switch (face) {
        case Face::FRONT: {
            // Front face vertices (x, y, z, u, v)
            std::vector frontVertices = {
                worldX - c_block_min, worldY + c_block_max, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_v_max,
                worldX + c_block_max, worldY + c_block_max, worldZ + c_block_max, u_base + c_texture_offset_66, v_base + c_texture_v_max,
                worldX + c_block_max, worldY - c_block_min, worldZ + c_block_max, u_base + c_texture_offset_66, v_base + c_texture_offset_0,
                worldX - c_block_min, worldY - c_block_min, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_offset_0
            };
            vertices.insert(vertices.end(), frontVertices.begin(), frontVertices.end());
            break;
        }
        case Face::BACK: {
            std::vector backVertices = {
                worldX - c_block_min, worldY + c_block_max, worldZ - c_block_min, u_base + c_texture_offset_33, v_base + c_texture_v_max,
                worldX + c_block_max, worldY + c_block_max, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_v_max,
                worldX + c_block_max, worldY - c_block_min, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_offset_0,
                worldX - c_block_min, worldY - c_block_min, worldZ - c_block_min, u_base + c_texture_offset_33, v_base + c_texture_offset_0
            };
            vertices.insert(vertices.end(), backVertices.begin(), backVertices.end());
            break;
        }
        case Face::LEFT: {
            std::vector leftVertices = {
                worldX - c_block_min, worldY + c_block_max, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_v_max,
                worldX - c_block_min, worldY + c_block_max, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_v_max,
                worldX - c_block_min, worldY - c_block_min, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_offset_0,
                worldX - c_block_min, worldY - c_block_min, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_offset_0
            };
            vertices.insert(vertices.end(), leftVertices.begin(), leftVertices.end());
            break;
        }
        case Face::RIGHT: {
            std::vector rightVertices = {
                worldX + c_block_max, worldY + c_block_max, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_v_max,
                worldX + c_block_max, worldY + c_block_max, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_v_max,
                worldX + c_block_max, worldY - c_block_min, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_offset_0,
                worldX + c_block_max, worldY - c_block_min, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_offset_0
            };
            vertices.insert(vertices.end(), rightVertices.begin(), rightVertices.end());
            break;
        }
        case Face::TOP: {
            std::vector<float> topVertices;
            topVertices.reserve(4 * 5); // Reserve space for 4 vertices, 5 components per vertex
            if (type == BlockType::WATER) {
                topVertices = {
                    worldX - c_block_min, worldY + c_block_max - 0.2f, worldZ + c_block_max, u_base + c_texture_offset_66, v_base + c_texture_v_max,
                    worldX + c_block_max, worldY + c_block_max - 0.2f, worldZ + c_block_max, u_base + c_texture_offset_100, v_base + c_texture_v_max,
                    worldX + c_block_max, worldY + c_block_max - 0.2f, worldZ - c_block_min, u_base + c_texture_offset_100, v_base + c_texture_offset_0,
                    worldX - c_block_min, worldY + c_block_max - 0.2f, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_offset_0
                };
            } else {
                topVertices = {
                    worldX - c_block_min, worldY + c_block_max, worldZ + c_block_max, u_base + c_texture_offset_66, v_base + c_texture_v_max,
                    worldX + c_block_max, worldY + c_block_max, worldZ + c_block_max, u_base + c_texture_offset_100, v_base + c_texture_v_max,
                    worldX + c_block_max, worldY + c_block_max, worldZ - c_block_min, u_base + c_texture_offset_100, v_base + c_texture_offset_0,
                    worldX - c_block_min, worldY + c_block_max, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_offset_0
                };
            }
            vertices.insert(vertices.end(), topVertices.begin(), topVertices.end());
            break;
        }
        case Face::BOTTOM: {
            std::vector bottomVertices = {
                worldX - c_block_min, worldY - c_block_min, worldZ + c_block_max, u_base + c_texture_offset_0, v_base + c_texture_v_max,
                worldX + c_block_max, worldY - c_block_min, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_v_max,
                worldX + c_block_max, worldY - c_block_min, worldZ - c_block_min, u_base + c_texture_offset_33, v_base + c_texture_offset_0,
                worldX - c_block_min, worldY - c_block_min, worldZ - c_block_min, u_base + c_texture_offset_0, v_base + c_texture_offset_0
            };
            vertices.insert(vertices.end(), bottomVertices.begin(), bottomVertices.end());
            break;
        }
        default:
            throw std::invalid_argument("Invalid face type");
    }

    return vertices;
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
    m_indices.reserve(6 * static_cast<unsigned int>(m_addedFaces.size())); // 6 indices per face
    for (const auto& face : m_addedFaces) {
        unsigned int baseIndex = static_cast<unsigned int>(std::distance(m_addedFaces.begin(), m_addedFaces.find(face))) * 4;

        switch (face) {
            case Face::FRONT:
            case Face::RIGHT:
            case Face::BOTTOM:
                m_indices.push_back(baseIndex);
                m_indices.push_back(baseIndex + 1);
                m_indices.push_back(baseIndex + 2);

                m_indices.push_back(baseIndex);
                m_indices.push_back(baseIndex + 2);
                m_indices.push_back(baseIndex + 3);
                break;
            case Face::BACK:
            case Face::LEFT:
            case Face::TOP:
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

    m_indices.shrink_to_fit();

    return m_indices.data();
}
