#ifndef BLOCK_H
#define BLOCK_H
#include <array>
#include <vector>

#include "GL/glew.h"

enum class Face : uint8_t {
    FRONT, BACK, LEFT, RIGHT, TOP, BOTTOM,
    TOP_INVERSED // TOP is drawn CW, TOP_INVERSED is drawn CCW
};

enum class BlockType : uint8_t {
    AIR, BEDROCK, DIRT, GRASS, STONE, WATER, LOG, LEAVES, SHORT_GRASS, FLOWER_POPPY, FLOWER_CORNFLOWER, FLOWER_ALLIUM,
    SNOW, SNOW_GRASS
};

struct BlockVertex {
    alignas(16) unsigned int position[3];
    alignas(16) unsigned int texCoords[2];
    unsigned int faceType;
    alignas(16) unsigned int AO[4];
};

struct HighlightedVertex {
    std::array<uint8_t, 3> position; // Position
    std::array<uint8_t, 3> color; // Color
};

class Block {
private:
    float m_x, m_y, m_z;
    static constexpr uint8_t s_textureColumn[14][3] = {
        // [side, top, bottom]
        {0, 0, 0}, // AIR
        {0, 0, 0}, // BEDROCK
        {1, 1, 1}, // DIRT
        {2, 3, 1}, // GRASS
        {4, 4, 4}, // STONE
        {0, 0, 0}, // WATER
        {3, 4, 4}, // LOG
        {0, 0, 0}, // LEAVES
        {1, 1, 1}, // SHORT_GRASS
        {2, 2, 2}, // POPPY
        {3, 3, 3}, // CORNFLOWER
        {4, 4, 4}, // ALLIUM
        {0, 0, 0}, // SNOW
        {1, 2, 1}  // SNOW_GRASS
    };
    static constexpr uint8_t s_textureRow[14][3] = {
        // [side, top, bottom]
        {0, 0, 0}, // AIR
        {4, 4, 4}, // BEDROCK
        {4, 4, 4}, // DIRT
        {4, 4, 4}, // GRASS
        {4, 4, 4}, // STONE
        {3, 3, 3}, // WATER
        {2, 2, 2}, // LOG
        {1, 1, 1}, // LEAVES
        {1, 1, 1}, // SHORT_GRASS
        {1, 1, 1}, // POPPY
        {1, 1, 1}, // CORNFLOWER
        {1, 1, 1}, // ALLIUM
        {0, 0, 0}, // SNOW
        {0, 0, 4}  // SNOW_GRASS
    };

public:
    Block(float x, float y, float z);

    static BlockType getBlockType(int y, int columnHeight);
    static const char *getBlockName(BlockType blockType);
    static void addFaceVertices(Face face, BlockType type, std::vector<BlockVertex> &vertices, const std::array<bool, 26> &adjacentsFaces, float block_startX, float block_startY, float block_startZ);
    static void addFaceVerticesAsBilboard(Face face, BlockType type, std::vector<BlockVertex> &vertices, float block_startX, float block_startY, float block_startZ);
    static uint8_t computeVertexAO(bool side1, bool side2, bool corner);

    static bool isTransparent(BlockType type);
    static bool isInstance(BlockType type);

private:
    static uint8_t getTextureU(BlockType type, Face face);
    static uint8_t getTextureV(BlockType type, Face face);
    [[nodiscard]] static int AOIndex(int x, int y, int z);
};

#endif //BLOCK_H
