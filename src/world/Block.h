#ifndef BLOCK_H
#define BLOCK_H
#include <array>
#include <cstdint>
#include <vector>

enum class Face : uint8_t {
    FRONT, BACK, LEFT, RIGHT, TOP, BOTTOM,
    TOP_INVERSED // TOP is drawn CW, TOP_INVERSED is drawn CCW
};

enum class BlockType : uint8_t {
    AIR, BEDROCK, DIRT, GRASS, STONE, WATER, LOG, LEAVES, SHORT_GRASS, FLOWER_POPPY, FLOWER_CORNFLOWER, FLOWER_ALLIUM,
    SNOW, SNOW_GRASS
};

struct BlockVertex {
    // Data[0]: position, texture layer, facetype, AO
    unsigned int packedData;
};

struct HighlightedVertex {
    std::array<uint8_t, 3> position; // Position
    std::array<uint8_t, 3> color; // Color
};

class Block {
private:
    float m_x, m_y, m_z;
    static constexpr uint8_t s_textureLayer[14][3] = {
        // [side, top, bottom]
        {0, 0, 0},    // AIR
        {0, 0, 0},    // BEDROCK
        {1, 1, 1},    // DIRT
        {2, 3, 1},    // GRASS
        {4, 4, 4},    // STONE
        {5, 5, 5},    // WATER
        {13, 14, 14}, // LOG
        {15, 15, 15}, // LEAVES
        {16, 16, 16}, // SHORT_GRASS
        {17, 17, 17}, // POPPY
        {18, 18, 18}, // CORNFLOWER
        {19, 19, 19}, // ALLIUM
        {20, 20, 20}, // SNOW
        {21, 22, 1}   // SNOW_GRASS
    };

public:
    Block(float x, float y, float z);

    static BlockType getBlockType(int y, int columnHeight);
    static const char *getBlockName(BlockType blockType);
    static void addFaceVertices(Face face, BlockType type, std::vector<BlockVertex> &vertices, const std::array<bool, 26> &adjacentsFaces, unsigned int startX, unsigned int startY, unsigned int startZ);
    static void addFaceVerticesAsBilboard(Face face, BlockType type, std::vector<BlockVertex> &vertices, unsigned int startX, unsigned int startY, unsigned int startZ);
    static uint8_t computeVertexAO(bool side1, bool side2, bool corner);

    static bool isTransparent(BlockType type);
    static bool isInstance(BlockType type);

private:
    static BlockVertex packVertexData(const unsigned int position[3], uint8_t texLayer, unsigned int faceIndex, const unsigned int ao[4]);
    static uint8_t getTextureLayer(BlockType type, Face face);
    [[nodiscard]] static int AOIndex(int x, int y, int z);
};

#endif //BLOCK_H
