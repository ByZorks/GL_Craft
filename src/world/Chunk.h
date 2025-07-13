#ifndef CHUNK_H
#define CHUNK_H
#include <vector>

#include "Block.h"
#include "../math/AABB.h"
#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"

struct BlockFaceData {
    face faceType;
    unsigned int vertexCount;
};

class Chunk {
private:
    static unsigned int m_size;
    int m_xStart, m_yStart, m_zStart;
    std::vector<float> m_vertices;
    const unsigned int *m_indices{};
    std::vector<BlockFaceData> m_blockFaceData;
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    IndexBuffer m_IBO;
    AABB m_box;

public:
    Chunk(int x, int y, int z);
    ~Chunk();

    void generate();
    void setupBuffers();

    [[nodiscard]] const VertexArray & m_vao() const;
    [[nodiscard]] const IndexBuffer & m_ibo() const;
    [[nodiscard]] static unsigned int m_size1() ;
    [[nodiscard]] const AABB & m_box1() const;
    [[nodiscard]] int m_x_start() const;
    [[nodiscard]] int m_y_start() const;
    [[nodiscard]] int m_z_start() const;
};

#endif //CHUNK_H
