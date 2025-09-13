#ifndef BLOCK_H
#define BLOCK_H
#include <array>
#include <cstdint>
#include <vector>

class Block {
public:
    enum class Face : uint8_t {
        FRONT, BACK, LEFT, RIGHT, TOP, BOTTOM,
    };

    // Same order as Face enum
    static constexpr std::array<std::tuple<int, int, int>, 6> s_faceOffset = {
        std::make_tuple(0, 0, 1),  // Front (+Z)
        std::make_tuple(0, 0, -1), // Back (-Z)
        std::make_tuple(-1, 0, 0), // Left (-X)
        std::make_tuple(1, 0, 0),  // Right (+X)
        std::make_tuple(0, 1, 0),  // Top (+Y)
        std::make_tuple(0, -1, 0)  // Bottom (-Y)
    };

    enum class BlockType : uint8_t {
        AIR, BEDROCK, DIRT, GRASS, STONE, WATER, OAK_LOG, OAK_LEAVES, SHORT_GRASS, FLOWER_POPPY, FLOWER_CORNFLOWER,
        FLOWER_ALLIUM, SNOW, SNOW_GRASS, SAND, GRAVEL, SNOW_OAK_LEAVES, CACTUS, JUNGLE_LOG, JUNGLE_LEAVES, JUNGLE_GRASS,
        SPRUCE_LOG, SPRUCE_LEAVES,
    };

    struct BlockVertex {
        // [0] position, texture layer, facetype, AO
        // [1] lighting
        uint32_t packedData[2];
    };

public:
    static const char *getBlockName(BlockType blockType);
    static void addFaceVertex(Face face, BlockType type, std::vector<BlockVertex> &outVertices,
                                const std::array<bool, 26> &adjacentsFaces, unsigned int startX, unsigned int startY,
                                unsigned int startZ, unsigned int lightLevel);
    static void addFaceVerticesAsBilboard(Face face, BlockType type, std::vector<BlockVertex> &outVertices,
                                          unsigned int startX, unsigned int startY, unsigned int startZ);
    static bool isOpaque(BlockType type);
    static bool isTransparent(BlockType type);
    static bool isSemiTransparent(BlockType type);
    static bool isInstance(BlockType type);
    static bool isLightEmitter(BlockType type);

private:
    static BlockVertex packVertexData(const unsigned int position[3], uint8_t texLayer, unsigned int faceIndex,
                                      const unsigned int ao[4], unsigned int lightLevel);

    static uint8_t getTextureLayer(BlockType type, Face face);
    static uint8_t computeVertexAO(bool side1, bool side2, bool corner);
    [[nodiscard]] static int AOIndex(int x, int y, int z);

private:
    static constexpr uint8_t s_textureLayer[24][3] = {
        // [side, top, bottom]
        {0, 0, 0},    // AIR
        {0, 0, 0},    // BEDROCK
        {1, 1, 1},    // DIRT
        {2, 3, 1},    // GRASS
        {4, 4, 4},    // STONE
        {5, 5, 5},    // WATER
        {13, 14, 14}, // OAK_LOG
        {15, 15, 15}, // OAK_LEAVES
        {16, 16, 16}, // SHORT_GRASS
        {17, 17, 17}, // POPPY
        {18, 18, 18}, // CORNFLOWER
        {19, 19, 19}, // ALLIUM
        {20, 20, 20}, // SNOW
        {21, 22, 1},  // SNOW_GRASS
        {24, 24, 24}, // SAND
        {25, 25, 25}, // GRAVEL
        {26, 26, 26}, // SNOW_OAK_LEAVES
        {27, 28, 28}, // CACTUS
        {29, 30, 30}, // JUNGLE_LOG
        {31, 31, 31}, // JUNGLE_LEAVES
        {32, 33, 1},  // JUNGLE_GRASS
        {34, 35, 35}, // SPRUCE_LOG
        {36, 36, 36}, // SPRUCE_LEAVES
    };
};

#endif //BLOCK_H
