#ifndef BLOCK_H
#define BLOCK_H
#include <unordered_set>
#include <vector>

enum class Face : unsigned int {
    FRONT = 0,
    BACK,
    LEFT,
    RIGHT,
    TOP,
    BOTTOM
};

enum class BlockType {
    BEDROCK,
    DIRT,
    GRASS,
    STONE,
    WATER
};

class Block {
private:
    float m_x, m_y, m_z, m_columnIndex;

public:
    Block(float x, float y, float z);
    ~Block();

    void setType(BlockType type);

    static void addFaceVertices(Face face, BlockType type, std::vector<float> *vertices, float worldX, float worldY, float worldZ, float u_base);
};

#endif //BLOCK_H
