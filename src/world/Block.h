#ifndef BLOCK_H
#define BLOCK_H
#include <vector>

#include "vec3.hpp"
#include "GL/glew.h"

enum class Face : uint8_t {
    FRONT, BACK, LEFT, RIGHT, TOP, BOTTOM,
    TOP_INVERSED // TOP is drawn CW, TOP_INVERSED is drawn CCW
};

struct BlockFaceData {
    Face faceType;
    uint8_t vertexCount;
    uint8_t x, y, z;

    [[nodiscard]] glm::vec3 getPosition() const {
        return {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
    }
};

enum class BlockType : uint8_t {
    AIR, BEDROCK, DIRT, GRASS, STONE, WATER, LOG, LEAVES, SHORT_GRASS, FLOWER_POPPY, FLOWER_CORNFLOWER, FLOWER_ALLIUM
};

struct BlockVertex {
    uint8_t x, y, z; // Position in local coordinates
    uint8_t texCol, texRow; // Texture row and column in the texture atlas
    uint8_t face; // Face index to determine the normal vector
};

class Block {
private:
    float m_x, m_y, m_z;
    static constexpr uint8_t s_textureColumn[12][3] = {
        // [side, top, bottom]
        {0, 0, 0}, // AIR
        {0, 0, 0}, // BEDROCK
        {1, 1, 1}, // DIRT
        {2, 3, 1}, // GRASS
        {4, 4, 4}, // STONE
        {0, 0 , 0}, // WATER
        {3, 4, 4}, // LOG
        {0, 0, 0}, // LEAVES
        {1, 1, 1}, // SHORT_GRASS
        {2, 2, 2}, // POPPY
        {3, 3, 3}, // CORNFLOWER
        {4, 4, 4}  // ALLIUM
    };
    static constexpr uint8_t s_textureRow[12][3] = {
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
        {1, 1, 1}  // ALLIUM
    };

public:
    Block(float x, float y, float z);

    static BlockType getBlockType(int y, int columnHeight);
    static void addFaceVertices(Face face, BlockType type, std::vector<BlockVertex> &vertices, float block_startX, float block_startY, float block_startZ);
    static void addFaceVerticesAsBilboard(Face face, BlockType type, std::vector<BlockVertex> &vertices, float block_startX, float block_startY, float block_startZ);

    static bool isTransparent(BlockType type);

private:
    static uint8_t getTextureU(BlockType type, Face face);
    static uint8_t getTextureV(BlockType type, Face face);
};

#endif //BLOCK_H
