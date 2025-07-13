#ifndef BLOCK_H
#define BLOCK_H
#include <unordered_set>
#include <vector>

enum face : unsigned int {
    FRONT = 0,
    BACK,
    LEFT,
    RIGHT,
    TOP,
    BOTTOM
};

class Block {
private:
    float m_x, m_y, m_z, m_index;
    std::vector<float> m_vertices;
    std::vector<unsigned int> m_indices;
    std::unordered_set<face> m_addedFaces;

public:
    Block(float x, float y, float z, float index);
    ~Block();

    void addFace(face face);

    [[nodiscard]] const float* getVertices() const;
    unsigned int getVertexCount() const;
    const unsigned int* getIndices();
    unsigned int getIndexCount() const;
};

#endif //BLOCK_H
