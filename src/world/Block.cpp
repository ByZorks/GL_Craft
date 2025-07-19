#include "Block.h"

#include <stdexcept>

#include "Chunk.h"

Block::Block(const float x, const float y, const float z) : m_x(x), m_y(y), m_z(z), m_columnIndex(0) {
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

BlockType Block::getBlockType(const int y) {
    if (y == 0) return BlockType::BEDROCK;
    if (y < 60) return BlockType::STONE;
    if (y == 60) return BlockType::WATER;
    if (y < 80) return BlockType::GRASS;
    return BlockType::STONE;
}

float Block::getTextureColumnIndex(const BlockType type) {
    float columnIndex;
    switch (type) {
        case BlockType::BEDROCK: columnIndex = 0; break;
        case BlockType::DIRT: columnIndex = 3; break;
        case BlockType::GRASS: columnIndex = 6; break;
        case BlockType::STONE: columnIndex = 9; break;
        case BlockType::WATER: columnIndex = 12; break;
        default: throw std::invalid_argument("Invalid block type");
    }
    return columnIndex;
}

void Block::addFaceVertices(const Face face, const BlockType type, std::vector<float> *vertices, const float worldX, const float worldY, const float worldZ, const float u_base) {
    constexpr float c_texture_width = 1.0f / 15.0f;
    constexpr float c_texture_offset_0 = 0.0f;
    constexpr float c_texture_offset_33 = c_texture_width;
    constexpr float c_texture_offset_66 = c_texture_width * 2.0f;
    constexpr float c_texture_offset_100 = c_texture_width * 3.0f;
    constexpr float c_texture_v_max = 1.0f;
    constexpr float c_block_min = 0.0f;
    constexpr float c_block_max = 1.0f;
    constexpr float v_base = 0.0f;

    // std::vector<float> vertices;
    vertices->reserve(vertices->size() + 4*8); // 4 vertices * 8 components per vertex

    // Helper lambda to add a vertex directly
    auto addVertex = [&vertices](float x, float y, float z, float u, float v, float nx, float ny, float nz) {
        vertices->emplace_back(x);
        vertices->emplace_back(y);
        vertices->emplace_back(z);
        vertices->emplace_back(u);
        vertices->emplace_back(v);
        vertices->emplace_back(nx);
        vertices->emplace_back(ny);
        vertices->emplace_back(nz);
    };

    switch (face) {
        case Face::FRONT: {
            constexpr float normal[3] = {0.f, 0.f, -1.f};
            addVertex(worldX - c_block_min, worldY + c_block_max, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_v_max, normal[0], normal[1], normal[2]);
            addVertex(worldX + c_block_max, worldY + c_block_max, worldZ + c_block_max, u_base + c_texture_offset_66, v_base + c_texture_v_max, normal[0], normal[1], normal[2]);
            addVertex(worldX + c_block_max, worldY - c_block_min, worldZ + c_block_max, u_base + c_texture_offset_66, v_base + c_texture_offset_0, normal[0], normal[1], normal[2]);
            addVertex(worldX - c_block_min, worldY - c_block_min, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_offset_0, normal[0], normal[1], normal[2]);
            break;
        }
        case Face::BACK: {
            constexpr float normal[3] = {0.f, 0.f, 1.f};
            addVertex(worldX - c_block_min, worldY + c_block_max, worldZ - c_block_min, u_base + c_texture_offset_33, v_base + c_texture_v_max, normal[0], normal[1], normal[2]);
            addVertex(worldX + c_block_max, worldY + c_block_max, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_v_max, normal[0], normal[1], normal[2]);
            addVertex(worldX + c_block_max, worldY - c_block_min, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_offset_0, normal[0], normal[1], normal[2]);
            addVertex(worldX - c_block_min, worldY - c_block_min, worldZ - c_block_min, u_base + c_texture_offset_33, v_base + c_texture_offset_0, normal[0], normal[1], normal[2]);
            break;
        }
        case Face::LEFT: {
            constexpr float normal[3] = {-1.f, 0.f, 0.f};
            addVertex(worldX - c_block_min, worldY + c_block_max, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_v_max, normal[0], normal[1], normal[2]);
            addVertex(worldX - c_block_min, worldY + c_block_max, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_v_max, normal[0], normal[1], normal[2]);
            addVertex(worldX - c_block_min, worldY - c_block_min, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_offset_0, normal[0], normal[1], normal[2]);
            addVertex(worldX - c_block_min, worldY - c_block_min, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_offset_0, normal[0], normal[1], normal[2]);
            break;
        }
        case Face::RIGHT: {
            constexpr float normal[3] = {1.f, 0.f, 0.f};
            addVertex(worldX + c_block_max, worldY + c_block_max, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_v_max, normal[0], normal[1], normal[2]);
            addVertex(worldX + c_block_max, worldY + c_block_max, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_v_max, normal[0], normal[1], normal[2]);
            addVertex(worldX + c_block_max, worldY - c_block_min, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_offset_0, normal[0], normal[1], normal[2]);
            addVertex(worldX + c_block_max, worldY - c_block_min, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_offset_0, normal[0], normal[1], normal[2]);
            break;
        }
        case Face::TOP: {
            constexpr float normal[3] = {0.f, 1.f, 0.f};
            const float topY = (type == BlockType::WATER) ? worldY + c_block_max - 0.2f : worldY + c_block_max;
            addVertex(worldX - c_block_min, topY, worldZ + c_block_max, u_base + c_texture_offset_66, v_base + c_texture_v_max, normal[0], normal[1], normal[2]);
            addVertex(worldX + c_block_max, topY, worldZ + c_block_max, u_base + c_texture_offset_100, v_base + c_texture_v_max, normal[0], normal[1], normal[2]);
            addVertex(worldX + c_block_max, topY, worldZ - c_block_min, u_base + c_texture_offset_100, v_base + c_texture_offset_0, normal[0], normal[1], normal[2]);
            addVertex(worldX - c_block_min, topY, worldZ - c_block_min, u_base + c_texture_offset_66, v_base + c_texture_offset_0, normal[0], normal[1], normal[2]);
            break;
        }
        case Face::BOTTOM: {
            constexpr float normal[3] = {0.f, -1.f, 0.f};
            addVertex(worldX - c_block_min, worldY - c_block_min, worldZ + c_block_max, u_base + c_texture_offset_0, v_base + c_texture_v_max, normal[0], normal[1], normal[2]);
            addVertex(worldX + c_block_max, worldY - c_block_min, worldZ + c_block_max, u_base + c_texture_offset_33, v_base + c_texture_v_max, normal[0], normal[1], normal[2]);
            addVertex(worldX + c_block_max, worldY - c_block_min, worldZ - c_block_min, u_base + c_texture_offset_33, v_base + c_texture_offset_0, normal[0], normal[1], normal[2]);
            addVertex(worldX - c_block_min, worldY - c_block_min, worldZ - c_block_min, u_base + c_texture_offset_0, v_base + c_texture_offset_0, normal[0], normal[1], normal[2]);
            break;
        }
        default:
            throw std::invalid_argument("Invalid face type");
    }
}

bool Block::isTransparent(const BlockType type) {
    return type == BlockType::WATER;
}
