#include "Block.h"

#include <array>
#include <stdexcept>

#include "chunk/Chunk.h"

const char *Block::getBlockName(const BlockType blockType) {
    switch (blockType) {
        case BlockType::AIR: return "AIR";
        case BlockType::BEDROCK: return "BEDROCK";
        case BlockType::DIRT: return "DIRT";
        case BlockType::GRASS: return "GRASS";
        case BlockType::STONE: return "STONE";
        case BlockType::WATER: return "WATER";
        case BlockType::OAK_LOG: return "LOG";
        case BlockType::OAK_LEAVES: return "LEAVES";
        case BlockType::SHORT_GRASS: return "SHORT_GRASS";
        case BlockType::FLOWER_POPPY: return "FLOWER_POPPY";
        case BlockType::FLOWER_CORNFLOWER: return "FLOWER_CORNFLOWER";
        case BlockType::FLOWER_ALLIUM: return "FLOWER_ALLIUM";
        case BlockType::SNOW: return "SNOW";
        case BlockType::SNOW_GRASS: return "SNOW_GRASS";
        case BlockType::SAND: return "SAND";
        case BlockType::GRAVEL: return "GRAVEL";
        case BlockType::SNOW_OAK_LEAVES: return "SNOW_OAK_LEAVES";
        case BlockType::CACTUS: return "CACTUS";
        case BlockType::JUNGLE_LOG: return "JUNGLE_LOG";
        case BlockType::JUNGLE_LEAVES: return "JUNGLE_LEAVES";
        case BlockType::JUNGLE_GRASS: return "JUNGLE_GRASS";
        case BlockType::SPRUCE_LEAVES: return "SPRUCE_LEAVES";
        case BlockType::SPRUCE_LOG: return "SPRUCE_LOG";
        case BlockType::RED_LIGHT: return "RED_LIGHT";
        case BlockType::GREEN_LIGHT: return "GREEN_LIGHT";
        case BlockType::BLUE_LIGHT: return "BLUE_LIGHT";
        case BlockType::PURPLE_LIGHT: return "PURPLE_LIGHT";
        case BlockType::PINK_LIGHT: return "PINK_LIGHT";
        case BlockType::YELLOW_LIGHT: return "YELLOW_LIGHT";
        default: return "UNKNOWN";
    }
}

void Block::addFaceVertex(const Face face, const BlockType type, std::vector<BlockVertex> &outVertices,
                            const std::array<bool, 26> &adjacentsFaces, const unsigned int startX,
                            const unsigned int startY, const unsigned int startZ, const uint8_t sunlight, const RGBLight& blockLight) {
    const unsigned int position[3] = {startX, startY, startZ};
    const uint8_t texLayer = getTextureLayer(type, face);

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
            outVertices.emplace_back(packVertexData(position, texLayer, faceIndex, ao, sunlight, blockLight));
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
            outVertices.emplace_back(packVertexData(position, texLayer, faceIndex, ao, sunlight, blockLight));
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
            outVertices.emplace_back(packVertexData(position, texLayer, faceIndex, ao, sunlight, blockLight));
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
            outVertices.emplace_back(packVertexData(position, texLayer, faceIndex, ao, sunlight, blockLight));
            break;
        }
        case Face::TOP: {
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
            outVertices.emplace_back(packVertexData(position, texLayer, faceIndex, ao, sunlight, blockLight));
            break;
        }
        case Face::BOTTOM: {
            constexpr unsigned int faceIndex = 5;
            const unsigned int ao[4] = {
                computeVertexAO(
                    adjacentsFaces[AOIndex(-1, -1, 0)],
                    adjacentsFaces[AOIndex(0, -1, 1)],
                    adjacentsFaces[AOIndex(-1, -1, 1)]),
                computeVertexAO(
                    adjacentsFaces[AOIndex(1, -1, 0)],
                    adjacentsFaces[AOIndex(0, -1, 1)],
                    adjacentsFaces[AOIndex(1, -1, 1)]),
                computeVertexAO(
                    adjacentsFaces[AOIndex(1, -1, 0)],
                    adjacentsFaces[AOIndex(0, -1, -1)],
                    adjacentsFaces[AOIndex(1, -1, -1)]),
                computeVertexAO(
                    adjacentsFaces[AOIndex(-1, -1, 0)],
                    adjacentsFaces[AOIndex(0, -1, -1)],
                    adjacentsFaces[AOIndex(-1, -1, -1)]),
            };
            outVertices.emplace_back(packVertexData(position, texLayer, faceIndex, ao, sunlight, blockLight));
            break;
        }
        default:
            throw std::invalid_argument("Invalid face type");
    }
}

void Block::addFaceVerticesAsBilboard(const Face face, const BlockType type, std::vector<BlockVertex> &outVertices,
                                      const unsigned int startX, const unsigned int startY, const unsigned int startZ) {
    const unsigned int position[3] = {startX, startY, startZ};
    const uint8_t texLayer = getTextureLayer(type, face);
    constexpr unsigned int ao[4] = {3, 3, 3, 3}; // AO is not used for billboards
    constexpr unsigned int sunlight = 15; // Max light level for now
    constexpr RGBLight blockLight = {0, 0, 0}; // No block light for now

    switch (face) {
        case Face::FRONT: {
            constexpr unsigned int faceIndex = 0;
            outVertices.emplace_back(packVertexData(position, texLayer, faceIndex, ao, sunlight, blockLight));
            break;
        }
        case Face::BACK: {
            constexpr unsigned int faceIndex = 1;
            outVertices.emplace_back(packVertexData(position, texLayer, faceIndex, ao, sunlight, blockLight));
            break;
        }
        default:
            throw std::invalid_argument("Invalid face type");
    }
}

uint8_t Block::computeVertexAO(const bool side1, const bool side2, const bool corner) {
    constexpr uint8_t AO_MAX = 3;

    if (side1 && side2) {
        constexpr uint8_t AO_MIN = 0;
        return AO_MIN;
    }
    return AO_MAX - (side1 + side2 + corner);
}

bool Block::isOpaque(const BlockType type) {
    return type != BlockType::AIR && !isTransparent(type);
}

bool Block::isTransparent(const BlockType type) {
    return type == BlockType::WATER || type == BlockType::AIR ||
           type == BlockType::OAK_LEAVES || type == BlockType::SNOW_OAK_LEAVES || type == BlockType::JUNGLE_LEAVES ||
           type == BlockType::SPRUCE_LEAVES || isInstance(type);
}

bool Block::isSemiTransparent(const BlockType type) {
    return type == BlockType::WATER || type == BlockType::OAK_LEAVES ||
           type == BlockType::SNOW_OAK_LEAVES || type == BlockType::JUNGLE_LEAVES || type == BlockType::SPRUCE_LEAVES;
}

bool Block::isInstance(const BlockType type) {
    return type == BlockType::SHORT_GRASS || type == BlockType::FLOWER_POPPY ||
           type == BlockType::FLOWER_CORNFLOWER || type == BlockType::FLOWER_ALLIUM;
}

bool Block::isLightEmitter(const BlockType type) {
    return type == BlockType::RED_LIGHT || type == BlockType::GREEN_LIGHT ||
           type == BlockType::BLUE_LIGHT || type == BlockType::PURPLE_LIGHT ||
           type == BlockType::PINK_LIGHT || type == BlockType::YELLOW_LIGHT;
}

RGBLight Block::getLightColor(const BlockType type) {
    switch (type) {
        case BlockType::RED_LIGHT:
            return {15, 0, 0};
        case BlockType::GREEN_LIGHT:
            return {0, 15, 0};
        case BlockType::BLUE_LIGHT:
            return {0, 0, 15};
        case BlockType::PURPLE_LIGHT:
            return {10, 0, 15};
        case BlockType::PINK_LIGHT:
            return {15, 5, 10};
        case BlockType::YELLOW_LIGHT:
            return {15, 15, 0};
        default:
            return {0, 0, 0};
    }
}

Block::BlockVertex Block::packVertexData(const unsigned int position[3], const uint8_t texLayer, const unsigned int faceIndex,
                                         const unsigned int ao[4], const uint8_t sunlight, const RGBLight& blockLight) {
    BlockVertex vertex{};

    constexpr unsigned int POS_MASK = 0x1F; // 5 bits, 0-31 range
    constexpr unsigned int TEX_MASK = 0x3F; // 6 bits, 0-63 range
    constexpr unsigned int FACE_MASK = 0x7; // 3 bits, 0-7 range
    constexpr unsigned int AO_MASK = 0x3; // 2 bits, 0-3 range

    // Position (15 bits)
    vertex.packedData[0] |= position[0] & POS_MASK;
    vertex.packedData[0] |= (position[1] & POS_MASK) << 5;
    vertex.packedData[0] |= (position[2] & POS_MASK) << 10;

    // TexLayer (6 bits)
    vertex.packedData[0] |= (texLayer & TEX_MASK) << 15;

    // FaceIndex (3 bits)
    vertex.packedData[0] |= (faceIndex & FACE_MASK) << 21;

    // AO (8 bits)
    vertex.packedData[0] |= (ao[0] & AO_MASK) << 24;
    vertex.packedData[0] |= (ao[1] & AO_MASK) << 26;
    vertex.packedData[0] |= (ao[2] & AO_MASK) << 28;
    vertex.packedData[0] |= (ao[3] & AO_MASK) << 30;

    constexpr unsigned int LIGHTING_MASK = 0xF; // 4 bits, 0-15 range

    // Lighting (16 bits)
    vertex.packedData[1] |= sunlight & LIGHTING_MASK;
    vertex.packedData[1] |= (blockLight.r & LIGHTING_MASK) << 4;
    vertex.packedData[1] |= (blockLight.g & LIGHTING_MASK) << 8;
    vertex.packedData[1] |= (blockLight.b & LIGHTING_MASK) << 12;

    return vertex;
}

uint8_t Block::getTextureLayer(BlockType type, const Face face) {
    const uint8_t typeIndex = static_cast<int>(type);
    const uint8_t faceIndex = face == Face::TOP ? 1 : face == Face::BOTTOM ? 2 : 0;
    return s_textureLayer[typeIndex][faceIndex];
}

int Block::AOIndex(const int x, const int y, const int z) {
    constexpr int stride = 3;
    constexpr int middleIndex = 13;
    const int idx = (x + 1) * stride * stride + (y + 1) * stride + (z + 1);
    return idx < middleIndex ? idx : idx - 1;
}
