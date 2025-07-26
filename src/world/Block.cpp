#include "Block.h"

#include <iostream>
#include <stdexcept>

#include "Chunk.h"

Block::Block(const float x, const float y, const float z) : m_x(x), m_y(y), m_z(z) {
}

Block::~Block() = default;

BlockType Block::getBlockType(const float y, const float columnHeight, const float caveShape, const float caveThreshold) {
    const int yf = static_cast<int>(std::floor(y));
    const int columnHeightf = static_cast<int>(std::floor(columnHeight));
    if (yf < 1) return BlockType::AIR;
    if (yf == 1) return BlockType::BEDROCK;
    if (caveShape > caveThreshold - 0.1f && caveShape < caveThreshold + 0.1f) return BlockType::AIR;
    if (yf == columnHeightf) return BlockType::GRASS;
    if (yf < columnHeightf - 4) return BlockType::STONE;
    if (yf < columnHeightf) return BlockType::DIRT;
    return BlockType::AIR;
}

void Block::addFaceVertices(const Face face, const BlockType type, std::vector<BlockVertex> &vertices, const float block_startX, const float block_startY, const float block_startZ) {
    const float block_endX = block_startX + 1.0f;
    const float block_endY = block_startY + 1.0f;
    const float block_endZ = block_startZ + 1.0f;

    vertices.reserve(vertices.size() + 4);

    // Helper lambda to add a vertex directly
    auto addVertex = [&vertices](const float x, const float y, const float z, const uint8_t u, const uint8_t v, const uint8_t faceIndex) {
        vertices.emplace_back(BlockVertex{
            static_cast<uint8_t>(x),
            static_cast<uint8_t>(y),
            static_cast<uint8_t>(z),
            u,
            v,
            faceIndex
        });
    };

    switch (face) {
        case Face::FRONT: {
            const uint8_t u_start = getTextureU(type, face);
            const uint8_t u_end = u_start + 1;
            const uint8_t v_start = getTextureV(type, face);
            const uint8_t v_end = v_start + 1;
            constexpr uint8_t faceIndex = 0;
            addVertex(block_startX, block_endY, block_endZ, u_start, v_end, faceIndex);
            addVertex(block_endX, block_endY, block_endZ, u_end, v_end, faceIndex);
            addVertex(block_endX, block_startY, block_endZ, u_end, v_start, faceIndex);
            addVertex(block_startX, block_startY, block_endZ, u_start, v_start, faceIndex);
            break;
        }
        case Face::BACK: {
            const uint8_t u_start = getTextureU(type, face);
            const uint8_t u_end = u_start + 1;
            const uint8_t v_start = getTextureV(type, face);
            const uint8_t v_end = v_start + 1;
            constexpr uint8_t normal = 1;
            addVertex(block_startX, block_endY, block_startZ, u_start, v_end, normal);
            addVertex(block_endX, block_endY, block_startZ, u_end, v_end, normal);
            addVertex(block_endX, block_startY, block_startZ, u_end, v_start, normal);
            addVertex(block_startX, block_startY, block_startZ, u_start, v_start, normal);
            break;
        }
        case Face::LEFT: {
            const uint8_t u_start = getTextureU(type, face);
            const uint8_t u_end = u_start + 1;
            const uint8_t v_start = getTextureV(type, face);
            const uint8_t v_end = v_start + 1;
            constexpr uint8_t normal = 2;
            addVertex(block_startX, block_endY, block_endZ, u_start, v_end, normal);
            addVertex(block_startX, block_endY, block_startZ, u_end, v_end, normal);
            addVertex(block_startX, block_startY, block_startZ, u_end, v_start, normal);
            addVertex(block_startX, block_startY, block_endZ, u_start, v_start, normal);
            break;
        }
        case Face::RIGHT: {
            const uint8_t u_start = getTextureU(type, face);
            const uint8_t u_end = u_start + 1;
            const uint8_t v_start = getTextureV(type, face);
            const uint8_t v_end = v_start + 1;
            constexpr uint8_t normal = 3;
            addVertex(block_endX, block_endY, block_endZ, u_start, v_end, normal);
            addVertex(block_endX, block_endY, block_startZ, u_end, v_end, normal);
            addVertex(block_endX, block_startY, block_startZ, u_end, v_start, normal);
            addVertex(block_endX, block_startY, block_endZ, u_start, v_start, normal);
            break;
        }
        case Face::TOP: {
            const uint8_t u_start = getTextureU(type, face);
            const uint8_t u_end = u_start + 1;
            const uint8_t v_start = getTextureV(type, face);
            const uint8_t v_end = v_start + 1;
            constexpr uint8_t normal = 4;
            const float topY = type == BlockType::WATER ? block_endY - 0.2f : block_endY;
            addVertex(block_startX, topY, block_endZ, u_end, v_end, normal);
            addVertex(block_endX, topY, block_endZ, u_start , v_end, normal);
            addVertex(block_endX, topY, block_startZ, u_start , v_start, normal);
            addVertex(block_startX, topY, block_startZ, u_end, v_start, normal);
            break;
        }
        case Face::BOTTOM: {
            const uint8_t u_start = getTextureU(type, face);
            const uint8_t u_end = u_start + 1;
            const uint8_t v_start = getTextureV(type, face);
            const uint8_t v_end = v_start + 1;
            constexpr uint8_t normal = 5;
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

uint8_t Block::getTextureU(BlockType type, const Face face) {
    const uint8_t typeIndex = static_cast<int>(type);
    const uint8_t faceIndex = face == Face::TOP ? 1 : face == Face::BOTTOM ? 2 : 0;
    return s_textureColumn[typeIndex][faceIndex];
}

uint8_t Block::getTextureV(BlockType type, const Face face) {
    const uint8_t typeIndex = static_cast<int>(type);
    const uint8_t faceIndex = face == Face::TOP ? 1 : face == Face::BOTTOM ? 2 : 0;
    return s_textureRow[typeIndex][faceIndex];
}
