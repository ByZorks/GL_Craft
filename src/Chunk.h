#ifndef CHUNK_H
#define CHUNK_H
#include <vector>

#include "IndexBuffer.h"
#include "VertexArray.h"

class Chunk {
private:
    unsigned int m_size = 16;
    std::vector<float> m_vertices;
    const unsigned int *m_indices{};
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    IndexBuffer m_IBO;
    VertexBufferLayout m_layout;

public:
    Chunk();
    ~Chunk();

    void generate();
    void setupBuffers();

    [[nodiscard]] const VertexArray & m_vao() const;

    [[nodiscard]] const IndexBuffer & m_ibo() const;
};

#endif //CHUNK_H
