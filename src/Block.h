#ifndef BLOCK_H
#define BLOCK_H

#include "IndexBuffer.h"
#include "VertexArray.h"

class Block {
private:
    float m_Vertices[120]{};
    const unsigned int m_Indices[36] = {
        // Front face
        0, 1, 2,
        2, 3, 0,
        // Back face
        4, 5, 6,
        6, 7, 4,
        // Left face
        8, 9, 10,
        10, 11, 8,
        // Right face
        12, 13, 14,
        14, 15, 12,
        // Top face
        16, 17, 18,
        18, 19, 16,
        // Bottom face
        20, 21, 22,
        22, 23, 20
    };
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    VertexBufferLayout m_layout;
    IndexBuffer m_IBO;

public:
    Block(float x, float y, float z, float index);
    ~Block();

    [[nodiscard]] const VertexArray & m_vao() const;
    [[nodiscard]] const IndexBuffer & m_ibo() const;
};

#endif //BLOCK_H
