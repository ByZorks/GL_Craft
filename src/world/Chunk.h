#ifndef CHUNK_H
#define CHUNK_H
#include <vector>

#include "Block.h"
#include "../math/AABB.h"
#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"

class World;

struct BlockFaceData {
    face faceType;
    unsigned int vertexCount;
};

enum status {
    NOT_GENERATED = 0,
    GENERATED = 1,
    MESH_GENERATED = 2,
    BUFFERS_SETUP = 3
};

class Chunk {
private:
    static unsigned int m_size;
    int m_xStart, m_yStart, m_zStart;
    std::vector<float> m_vertices;
    const unsigned int *m_indices{};
    std::vector<BlockFaceData> m_blockFaceData;
    bool m_blockPresent[16][16][16] = {{{false}}}; // 16x16x16 chunk size
    status m_status = NOT_GENERATED;
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    IndexBuffer m_IBO;
    AABB m_box;

public:
    Chunk(int x, int y, int z);
    ~Chunk();

    void generateVoxelData();
    void generateMeshData(const World * world);
    void setupBuffers();
    static bool isBlockPresentInWorld(float worldX, float worldY, float worldZ, const World *world) ;

    [[nodiscard]] const VertexArray & m_vao() const;
    [[nodiscard]] const IndexBuffer & m_ibo() const;
    [[nodiscard]] static unsigned int m_size1() ;
    [[nodiscard]] const AABB & m_box1() const;
    [[nodiscard]] int m_x_start() const;
    [[nodiscard]] int m_y_start() const;
    [[nodiscard]] int m_z_start() const;
    [[nodiscard]] bool isBlockPresentInLocal(int localX, int localY, int localZ) const;
    [[nodiscard]] status m_status1() const;
};

#endif //CHUNK_H
