#include "Block.h"

#include <stdexcept>

#include "Chunk.h"

Block::Block(const float x, const float y, const float z) : m_x(x), m_y(y), m_z(z), m_columnIndex(0) {
}

Block::~Block() = default;

BlockType Block::getBlockType(const int y, const int columnHeight) {
    if (y < 0) return BlockType::AIR;
    if (y == 0) return BlockType::BEDROCK;
    if (y < columnHeight - 4) return BlockType::STONE;
    if (y < columnHeight) return BlockType::DIRT;
    if (y == columnHeight) return BlockType::GRASS;
    return BlockType::AIR;
}

void Block::addFaceVertices(const Face face, const BlockType type, std::vector<BlockVertex> &vertices, const float block_startX, const float block_startY, const float block_startZ) {
    const float block_endX = block_startX + 1.0f;
    const float block_endY = block_startY + 1.0f;
    const float block_endZ = block_startZ + 1.0f;
    constexpr float v_start = 0.0f;
    constexpr float v_end = 1.0f;

    vertices.reserve(vertices.size() + 4);

    // Helper lambda to add a vertex directly
    auto addVertex = [&vertices](const float x, const float y, const float z, const float u, const float v, const float normal[3]) {
        vertices.emplace_back(BlockVertex{x, y, z, u, v, normal[0], normal[1], normal[2]});
    };

    switch (face) {
        case Face::FRONT: {
            const float u_start = getTextureU(type, face);
            const float u_end = u_start + s_textureWidth;
            constexpr float normal[3] = {0.f, 0.f, -1.f};
            addVertex(block_startX, block_endY, block_endZ, u_start, v_end, normal);
            addVertex(block_endX, block_endY, block_endZ, u_end, v_end, normal);
            addVertex(block_endX, block_startY, block_endZ, u_end, v_start, normal);
            addVertex(block_startX, block_startY, block_endZ, u_start, v_start, normal);
            break;
        }
        case Face::BACK: {
            const float u_start = getTextureU(type, face);
            const float u_end = u_start + s_textureWidth;
            constexpr float normal[3] = {0.f, 0.f, 1.f};
            addVertex(block_startX, block_endY, block_startZ, u_start, v_end, normal);
            addVertex(block_endX, block_endY, block_startZ, u_end, v_end, normal);
            addVertex(block_endX, block_startY, block_startZ, u_end, v_start, normal);
            addVertex(block_startX, block_startY, block_startZ, u_start, v_start, normal);
            break;
        }
        case Face::LEFT: {
            const float u_start = getTextureU(type, face);
            const float u_end = u_start + s_textureWidth;
            constexpr float normal[3] = {-1.f, 0.f, 0.f};
            addVertex(block_startX, block_endY, block_endZ, u_start, v_end, normal);
            addVertex(block_startX, block_endY, block_startZ, u_end, v_end, normal);
            addVertex(block_startX, block_startY, block_startZ, u_end, v_start, normal);
            addVertex(block_startX, block_startY, block_endZ, u_start, v_start, normal);
            break;
        }
        case Face::RIGHT: {
            const float u_start = getTextureU(type, face);
            const float u_end = u_start + s_textureWidth;
            constexpr float normal[3] = {1.f, 0.f, 0.f};
            addVertex(block_endX, block_endY, block_endZ, u_start, v_end, normal);
            addVertex(block_endX, block_endY, block_startZ, u_end, v_end, normal);
            addVertex(block_endX, block_startY, block_startZ, u_end, v_start, normal);
            addVertex(block_endX, block_startY, block_endZ, u_start, v_start, normal);
            break;
        }
        case Face::TOP: {
            const float u_start = getTextureU(type, face);
            const float u_end = u_start + s_textureWidth;
            constexpr float normal[3] = {0.f, 1.f, 0.f};
            const float topY = type == BlockType::WATER ? block_endY - 0.2f : block_endY;
            addVertex(block_startX, topY, block_endZ, u_end, v_end, normal);
            addVertex(block_endX, topY, block_endZ, u_start , v_end, normal);
            addVertex(block_endX, topY, block_startZ, u_start , v_start, normal);
            addVertex(block_startX, topY, block_startZ, u_end, v_start, normal);
            break;
        }
        case Face::BOTTOM: {
            const float u_start = getTextureU(type, face);
            const float u_end = u_start + s_textureWidth;
            constexpr float normal[3] = {0.f, -1.f, 0.f};
            addVertex(block_startX, block_startY, block_endZ, u_start, v_end, normal);
            addVertex(block_endX, block_startY, block_endZ, u_end, v_end, normal);
            addVertex(block_endX, block_startY, block_startZ, u_end, v_start, normal);
            addVertex(block_startX, block_startY, block_startZ, u_start, v_start, normal);
            break;
        }
        default:
            throw std::invalid_argument("Invalid face type");
    }
}

bool Block::isTransparent(const BlockType type) {
    return type == BlockType::WATER || type == BlockType::AIR;
}

float Block::getTextureU(BlockType type, const Face face) {
    const int typeIndex = static_cast<int>(type);
    const int faceIndex = face == Face::TOP ? 1 : face == Face::BOTTOM ? 2 : 0;
    return s_textureIndicesU[typeIndex][faceIndex];
}