#ifndef BLOCK_H
#define BLOCK_H
#include <vector>

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
    float x, y, z; // Position in world coordinates
    float u, v; // Texture coordinates
    float nx, ny, nz; // Normal vector components
};

class Block {
private:
    float m_x, m_y, m_z, m_columnIndex;
    static constexpr float s_textureWidth = 1.0f / 7.0f;
    static constexpr float s_textureIndicesU[6][3] = {
        // [side, top, bottom]
        {0, 0, 0}, // AIR
        {0, 0, 0}, // BEDROCK
        {s_textureWidth, s_textureWidth, s_textureWidth}, // DIRT
        {2 * s_textureWidth, 3 * s_textureWidth, s_textureWidth}, // GRASS
        {4 * s_textureWidth, 4 * s_textureWidth, 4 * s_textureWidth}, // STONE
        {5 * s_textureWidth, 6 * s_textureWidth, 6 * s_textureWidth} // WATER
    };

public:
    Block(float x, float y, float z);
    ~Block();

    static BlockType getBlockType(int y, int columnHeight);
    static void addFaceVertices(Face face, BlockType type, std::vector<BlockVertex> &vertices, float block_startX, float block_startY, float block_startZ);
    static bool isTransparent(BlockType type);
    static float getTextureU(BlockType type, Face face);
};

#endif //BLOCK_H
