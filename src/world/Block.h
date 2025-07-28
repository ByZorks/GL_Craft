#ifndef BLOCK_H
#define BLOCK_H
#include <vector>

#include "GL/glew.h"

enum class Face : uint8_t {
    FRONT,
    BACK,
    LEFT,
    RIGHT,
    TOP,
    BOTTOM
};

struct BlockFaceData {
    Face faceType;
    uint8_t vertexCount;
    bool transparent;
};

enum class BlockType : uint8_t {
    AIR, BEDROCK, DIRT, GRASS, STONE, WATER, LOG, LEAVES
};

struct BlockVertex {
    uint8_t x, y, z; // Position in world coordinates
    uint8_t texCol, texRow; // Texture coordinates
    uint8_t face;
};

class Block {
private:
    float m_x, m_y, m_z;
    static constexpr uint8_t s_textureColumn[8][3] = {
        // [side, top, bottom]
        {0, 0, 0}, // AIR
        {0, 0, 0}, // BEDROCK
        {1, 1, 1}, // DIRT
        {2, 3, 1}, // GRASS
        {0, 0, 0}, // STONE
        {1, 2, 2}, // WATER
        {3, 0, 0},  // LOG
        {1, 1, 1}  // LEAVES
    };
    static constexpr uint8_t s_textureRow[8][3] = {
        // [side, top, bottom]
        {0, 0, 0}, // AIR
        {3, 3, 3}, // BEDROCK
        {3, 3, 3}, // DIRT
        {3, 3, 3}, // GRASS
        {2, 2, 2}, // STONE
        {2, 2, 2}, // WATER
        {2, 1, 1},  // LOG
        {1, 1, 1}   // LEAVES
    };

public:
    Block(float x, float y, float z);
    ~Block();

    static BlockType getBlockType(float y, float columnHeight, float caveShape, float caveThreshold);
    static void addFaceVertices(Face face, BlockType type, std::vector<BlockVertex> &vertices, float block_startX, float block_startY, float block_startZ);
    static bool isTransparent(BlockType type);

private:
    static uint8_t getTextureU(BlockType type, Face face);
    static uint8_t getTextureV(BlockType type, Face face);
};

#endif //BLOCK_H
