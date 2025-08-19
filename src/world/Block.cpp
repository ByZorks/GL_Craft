#include "Block.h"

#include <array>
#include <stdexcept>

#include "Chunk.h"

Block::Block(const float x, const float y, const float z) : m_x(x), m_y(y), m_z(z) {
}

BlockType Block::getBlockType(const int y, const int columnHeight) {
    constexpr int waterLevel = 63;

    if (y < 1) return BlockType::AIR;
    if (y == 1) return BlockType::BEDROCK;

    if (y <= columnHeight) {
        // Surface block
        if (y == columnHeight && columnHeight >= waterLevel && y < 150) return BlockType::GRASS;
        if (y == columnHeight && columnHeight >= waterLevel && y < 200) return BlockType::SNOW_GRASS;
        if (y == columnHeight && columnHeight >= waterLevel) return BlockType::SNOW;
        if (y == columnHeight) return BlockType::DIRT; // Disallow cave entrances underwater

        // Subsurface blocks
        if (y < columnHeight - 4) return BlockType::STONE;

        // Near-surface blocks
        if (y < columnHeight && y < 200) return BlockType::DIRT;
        if (y < columnHeight) return BlockType::SNOW;
    }

    if (y > columnHeight && y <= waterLevel) {
        return BlockType::WATER;
    }

    return BlockType::AIR;
}

const char *Block::getBlockName(const BlockType blockType) {
    switch (blockType) {
        case BlockType::AIR: return "AIR";
        case BlockType::BEDROCK: return "BEDROCK";
        case BlockType::DIRT: return "DIRT";
        case BlockType::GRASS: return "GRASS";
        case BlockType::STONE: return "STONE";
        case BlockType::WATER: return "WATER";
        case BlockType::LOG: return "LOG";
        case BlockType::LEAVES: return "LEAVES";
        case BlockType::SHORT_GRASS: return "SHORT_GRASS";
        case BlockType::FLOWER_POPPY: return "FLOWER_POPPY";
        case BlockType::FLOWER_CORNFLOWER: return "FLOWER_CORNFLOWER";
        case BlockType::FLOWER_ALLIUM: return "FLOWER_ALLIUM";
        case BlockType::SNOW: return "SNOW";
        case BlockType::SNOW_GRASS: return "SNOW_GRASS";
        default: return "UNKNOWN";
    }
}

void Block::addFaceVertices(const Face face, const BlockType type, std::vector<BlockVertex> &vertices,
                            const std::array<bool, 26> &adjacentsFaces, const unsigned int startX,
                            const unsigned int startY, const unsigned int startZ) {

    const unsigned int position[3] = {startX, startY, startZ};
    const unsigned int texCoords[2] = {getTextureU(type, face), getTextureV(type, face)};

    switch (face) {
        case Face::FRONT: {
            constexpr unsigned int faceIndex = 0;
            const unsigned int ao[4] = {
                computeVertexAO(
                    adjacentsFaces[AOIndex(-1, 0, 1)],
                    adjacentsFaces[AOIndex(0, 1, 1)],
                    adjacentsFaces[AOIndex(-1, 1, 1)]),
                computeVertexAO(
                    adjacentsFaces[AOIndex(1, 0, 1)],
                    adjacentsFaces[AOIndex(0, 1, 1)],
                    adjacentsFaces[AOIndex(1, 1, 1)]),
                computeVertexAO(
                    adjacentsFaces[AOIndex(1, 0, 1)],
                    adjacentsFaces[AOIndex(0, -1, 1)],
                    adjacentsFaces[AOIndex(1, -1, 1)]),
                computeVertexAO(
                    adjacentsFaces[AOIndex(-1, 0, 1)],
                    adjacentsFaces[AOIndex(0, -1, 1)],
                    adjacentsFaces[AOIndex(-1, -1, 1)])
            };
            vertices.emplace_back(packVertexData(position, texCoords, faceIndex, ao));
            break;
        }
        case Face::BACK: {
            constexpr unsigned int faceIndex = 1;
            const unsigned int ao[4] = {
                computeVertexAO(
                          adjacentsFaces[AOIndex(1, 0, -1)],
                          adjacentsFaces[AOIndex(0, 1, -1)],
                          adjacentsFaces[AOIndex(1, 1, -1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(-1, 0, -1)],
                          adjacentsFaces[AOIndex(0, 1, -1)],
                          adjacentsFaces[AOIndex(-1, 1, -1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(-1, 0, -1)],
                          adjacentsFaces[AOIndex(0, -1, -1)],
                          adjacentsFaces[AOIndex(-1, -1, -1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(1, 0, -1)],
                          adjacentsFaces[AOIndex(0, -1, -1)],
                          adjacentsFaces[AOIndex(1, -1, -1)])
            };
            vertices.emplace_back(packVertexData(position, texCoords, faceIndex, ao));
            break;
        }
        case Face::LEFT: {
            constexpr unsigned int faceIndex = 2;
            const unsigned int ao[4] = {
                computeVertexAO(
                          adjacentsFaces[AOIndex(-1, 0, -1)],
                          adjacentsFaces[AOIndex(-1, 1, 0)],
                          adjacentsFaces[AOIndex(-1, 1, -1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(-1, 0, 1)],
                          adjacentsFaces[AOIndex(-1, 1, 0)],
                          adjacentsFaces[AOIndex(-1, 1, 1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(-1, 0, 1)],
                          adjacentsFaces[AOIndex(-1, -1, 0)],
                          adjacentsFaces[AOIndex(-1, -1, 1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(-1, 0, -1)],
                          adjacentsFaces[AOIndex(-1, -1, 0)],
                          adjacentsFaces[AOIndex(-1, -1, -1)])
            };
            vertices.emplace_back(packVertexData(position, texCoords, faceIndex, ao));
            break;
        }
        case Face::RIGHT: {
            constexpr unsigned int faceIndex = 3;
            const unsigned int ao[4] = {
                computeVertexAO(
                          adjacentsFaces[AOIndex(1, 0, 1)],
                          adjacentsFaces[AOIndex(1, 1, 0)],
                          adjacentsFaces[AOIndex(1, 1, 1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(1, 0, -1)],
                          adjacentsFaces[AOIndex(1, 1, 0)],
                          adjacentsFaces[AOIndex(1, 1, -1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(1, 0, -1)],
                          adjacentsFaces[AOIndex(1, -1, 0)],
                          adjacentsFaces[AOIndex(1, -1, -1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(1, 0, 1)],
                          adjacentsFaces[AOIndex(1, -1, 0)],
                          adjacentsFaces[AOIndex(1, -1, 1)])
            };
            vertices.emplace_back(packVertexData(position, texCoords, faceIndex, ao));
            break;
        }
        case Face::TOP:
        case Face::TOP_INVERSED: {
            // const unsigned int faceIndex = face == Face::TOP ? 4 : 6;
            constexpr unsigned int faceIndex = 4; // TOP face
            const unsigned int ao[4] = {
                computeVertexAO(
                          adjacentsFaces[AOIndex(-1, 1, 0)],
                          adjacentsFaces[AOIndex(0, 1, -1)],
                          adjacentsFaces[AOIndex(-1, 1, -1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(1, 1, 0)],
                          adjacentsFaces[AOIndex(0, 1, -1)],
                          adjacentsFaces[AOIndex(1, 1, -1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(1, 1, 0)],
                          adjacentsFaces[AOIndex(0, 1, 1)],
                          adjacentsFaces[AOIndex(1, 1, 1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(-1, 1, 0)],
                          adjacentsFaces[AOIndex(0, 1, 1)],
                          adjacentsFaces[AOIndex(-1, 1, 1)]),
            };
            vertices.emplace_back(packVertexData(position, texCoords, faceIndex, ao));
            break;
        }
        case Face::BOTTOM: {
            constexpr unsigned int faceIndex = 5;
            const unsigned int ao[4] = {
                computeVertexAO(
                          adjacentsFaces[AOIndex(-1, -1, 0)],
                          adjacentsFaces[AOIndex(0, -1, -1)],
                          adjacentsFaces[AOIndex(-1, -1, -1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(1, -1, 0)],
                          adjacentsFaces[AOIndex(0, -1, -1)],
                          adjacentsFaces[AOIndex(1, -1, -1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(1, -1, 0)],
                          adjacentsFaces[AOIndex(0, -1, 1)],
                          adjacentsFaces[AOIndex(1, -1, 1)]),
                computeVertexAO(
                          adjacentsFaces[AOIndex(-1, -1, 0)],
                          adjacentsFaces[AOIndex(0, -1, 1)],
                          adjacentsFaces[AOIndex(-1, -1, 1)]),
            };
            vertices.emplace_back(packVertexData(position, texCoords, faceIndex, ao));
            break;
        }
        default:
            throw std::invalid_argument("Invalid face type");
    }
}

void Block::addFaceVerticesAsBilboard(const Face face, const BlockType type, std::vector<BlockVertex> &vertices,
                                      const unsigned int startX, const unsigned int startY, const unsigned int startZ) {
    const unsigned int position[3] = {startX, startY, startZ};
    const unsigned int texCoords[2] = {getTextureU(type, face), getTextureV(type, face)};
    constexpr unsigned int ao[4] = {3, 3, 3, 3}; // AO is not used for billboards

    switch (face) {
        case Face::FRONT: {
            constexpr unsigned int faceIndex = 0;
            vertices.emplace_back(packVertexData(position, texCoords, faceIndex, ao));
            break;
        }
        case Face::BACK: {
            constexpr unsigned int faceIndex = 1;
            vertices.emplace_back(packVertexData(position, texCoords, faceIndex, ao));
            break;
        }
        default:
            throw std::invalid_argument("Invalid face type");
    }
}

uint8_t Block::computeVertexAO(const bool side1, const bool side2, const bool corner) {
    constexpr uint8_t AO_MIN = 0;
    constexpr uint8_t AO_MAX = 3;

    if (side1 && side2) {
        return AO_MIN;
    }
    return AO_MAX - (side1 + side2 + corner);
}

bool Block::isTransparent(const BlockType type) {
    return type == BlockType::WATER || type == BlockType::AIR || type == BlockType::LEAVES || isInstance(type);
}

bool Block::isInstance(const BlockType type) {
    return type == BlockType::SHORT_GRASS || type == BlockType::FLOWER_POPPY ||
           type == BlockType::FLOWER_CORNFLOWER || type == BlockType::FLOWER_ALLIUM;
}

BlockVertex Block::packVertexData(const unsigned int position[3], const unsigned int texCoords[2], const unsigned int faceIndex,
    const unsigned int ao[4]) {
    BlockVertex vertex = {0, 0};

    constexpr unsigned int POS_MASK = 0x1F; // 5 bits
    constexpr unsigned int FACE_MASK = 0x7; // 3 bits
    constexpr unsigned int TEX_MASK = 0xF; // 4 bits
    constexpr unsigned int AO_MASK = 0x3; // 2 bits

    // Position (15 bits) + Face (3 bits) = 18 bits
    vertex.packedData[0] |= position[0] & POS_MASK;
    vertex.packedData[0] |= (position[1] & POS_MASK) << 5;
    vertex.packedData[0] |= (position[2] & POS_MASK) << 13;
    vertex.packedData[0] |= (faceIndex & FACE_MASK) << 18;

    // TexCoords (8 bits) + AO (8 bits) = 16 bits
    vertex.packedData[1] |= texCoords[0] & TEX_MASK;
    vertex.packedData[1] |= (texCoords[1] & TEX_MASK) << 4;
    vertex.packedData[1] |= (ao[0] & AO_MASK) << 8;
    vertex.packedData[1] |= (ao[1] & AO_MASK) << 10;
    vertex.packedData[1] |= (ao[2] & AO_MASK) << 12;
    vertex.packedData[1] |= (ao[3] & AO_MASK) << 14;

    return vertex;
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

int Block::AOIndex(const int x, const int y, const int z) {
    constexpr int stride = 3;
    constexpr int middleIndex = 13;
    const int idx = (x + 1) * stride * stride + (y + 1) * stride + (z + 1);
    return idx < middleIndex ? idx : idx - 1;
}
