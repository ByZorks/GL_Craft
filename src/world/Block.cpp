#include "Block.h"

#include <stdexcept>

#include "Chunk.h"

Block::Block(const float x, const float y, const float z) : m_x(x), m_y(y), m_z(z) {
}

BlockType Block::getBlockType(const int y, const int columnHeight) {
    constexpr int waterLevel = 63;

    if (y < 1) return BlockType::AIR;
    if (y == 1) return BlockType::BEDROCK;

    if (y <= columnHeight) {
        if (y == columnHeight && columnHeight >= waterLevel) return BlockType::GRASS;
        if (y == columnHeight && columnHeight < waterLevel) return BlockType::DIRT;
        if (y < columnHeight - 4) return BlockType::STONE;
        if (y < columnHeight) return BlockType::DIRT;
    }

    if (y > columnHeight && y <= waterLevel) {
        return BlockType::WATER;
    }

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
        case Face::TOP: case Face::TOP_INVERSED: {
            const uint8_t u_start = getTextureU(type, face);
            const uint8_t u_end = u_start + 1;
            const uint8_t v_start = getTextureV(type, face);
            const uint8_t v_end = v_start + 1;
            constexpr uint8_t normal = 4;
            addVertex(block_startX, block_endY, block_endZ, u_start, v_end, normal);
            addVertex(block_endX, block_endY, block_endZ, u_end , v_end, normal);
            addVertex(block_endX, block_endY, block_startZ, u_end , v_start, normal);
            addVertex(block_startX, block_endY, block_startZ, u_start, v_start, normal);
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

void Block::addFaceVerticesAsBilboard(const Face face, const BlockType type, std::vector<BlockVertex> &vertices, const float block_startX,
    const float block_startY, const float block_startZ) {

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
            addVertex(block_endX, block_endY, block_startZ, u_end, v_end, faceIndex);
            addVertex(block_endX, block_startY, block_startZ, u_end, v_start, faceIndex);
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
            addVertex(block_endX, block_endY, block_endZ, u_end, v_end, normal);
            addVertex(block_endX, block_startY, block_endZ, u_end, v_start, normal);
            addVertex(block_startX, block_startY, block_startZ, u_start, v_start, normal);
            break;
        }
        default:
            throw std::invalid_argument("Invalid face type");
    }
}

bool Block::isTransparent(const BlockType type) {
    return type == BlockType::WATER || type == BlockType::AIR || type == BlockType::LEAVES;
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
