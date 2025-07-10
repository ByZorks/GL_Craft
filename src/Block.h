#ifndef BLOCK_H
#define BLOCK_H

class Block {
private:
    float m_Vertices[120]{};

public:
    Block(float x, float y, float z, float index);
    ~Block();

    static const unsigned int* getIndices();
    [[nodiscard]] const float* getVertices() const;
};

#endif //BLOCK_H
