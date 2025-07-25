#ifndef CHUNK_H
#define CHUNK_H
#include <vector>

#include "Block.h"
#include "FastNoiseLite.h"
#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"
#include "../math/AABB.h"

class World;

struct BlockFaceData {
    Face faceType;
    uint8_t vertexCount;
};

enum class Status : uint8_t {
    NOT_GENERATED,
    VOXEL_GENERATED,
    MESH_GENERATED,
    BUFFERS_SETUP
};

class Chunk {
private:
    static unsigned int m_size;
    int m_x, m_y, m_z;
    std::vector<BlockVertex> m_vertices;
    std::vector<BlockFaceData> m_blockFaceData;
    std::vector<BlockType> m_blockType;
    Status m_status = Status::NOT_GENERATED;
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    IndexBuffer m_IBO;
    AABB m_box;

public:
    Chunk(int x, int y, int z);
    ~Chunk();

    void generateVoxelData(const FastNoiseLite& noiseGenerator);
    void generateMeshData();
    void setupBuffers();

    static int index(int x, int y, int z);

    [[nodiscard]] bool hasBlocks();
    [[nodiscard]] bool hasVisibleFaces() const;

    [[nodiscard]] const VertexArray & m_vao() const;
    [[nodiscard]] const IndexBuffer & m_ibo() const;
    [[nodiscard]] static unsigned int m_size1() ;
    [[nodiscard]] const AABB & m_box1() const;
    [[nodiscard]] int m_x1() const;
    [[nodiscard]] int m_y1() const;
    [[nodiscard]] int m_z1() const;
    [[nodiscard]] Status m_status1() const;

private:
    void addBlockFaces(int localX, int localY, int localZ, BlockType blockType);
    [[nodiscard]] bool shouldDrawFace(int localX, int localY, int localZ, bool currentTransparent) const;
    [[nodiscard]] BlockType getBlockType(int localX, int localY, int localZ) const;
    [[nodiscard]] bool isBlockPresent(int localX, int localY, int localZ) const;
};

#endif //CHUNK_H
