#ifndef BLOCK_H
#define BLOCK_H
#include <vector>

#include "GL/glew.h"

enum class Face : unsigned int {
    FRONT = 0,
    BACK,
    LEFT,
    RIGHT,
    TOP,
    BOTTOM
};

enum class BlockType : unsigned int {
    AIR,
    BEDROCK,
    DIRT,
    GRASS,
    STONE,
    WATER,
};

struct BlockVertex {
    uint8_t x, y, z; // Position in world coordinates
    uint8_t texCol, texRow; // Texture coordinates
    int8_t nx, ny, nz; // Normal vector components
};

class Block {
private:
    float m_x, m_y, m_z, m_columnIndex;
    static constexpr float s_textureWidth = 255.f / 4.f;
    static constexpr uint8_t s_textureColumn[6][3] = {
        // [side, top, bottom]
        {0, 0, 0}, // AIR
        {0, 0, 0}, // BEDROCK
        {1, 1, 1}, // DIRT
        {2, 3, 1}, // GRASS
        {0, 0, 0}, // STONE
        {1, 2, 2} // WATER
    };
    static constexpr uint8_t s_textureRow[6][3] = {
        // [side, top, bottom]
        {3, 3, 3}, // AIR
        {3, 3, 3}, // BEDROCK
        {3, 3, 3}, // DIRT
        {3, 3, 3}, // GRASS
        {2, 2, 2}, // STONE
        {2, 2, 2} // WATER
    };

public:
    Block(float x, float y, float z);
    ~Block();

    static BlockType getBlockType(int y, int columnHeight);
    static void addFaceVertices(Face face, BlockType type, std::vector<BlockVertex> &vertices, float block_startX, float block_startY, float block_startZ);
    static bool isTransparent(BlockType type);

private:
    static uint8_t getTextureU(BlockType type, Face face);
    static uint8_t getTextureV(BlockType type, Face face);
};

#endif //BLOCK_H
