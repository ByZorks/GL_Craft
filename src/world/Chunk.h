#ifndef CHUNK_H
#define CHUNK_H
#include <memory>
#include <unordered_map>
#include <vector>

#include "Block.h"
#include "FastNoiseLite.h"
#include "../math/AABB.h"
#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"
#include "../utils/CustomHash.h"

class World;

struct BlockFaceData {
    Face faceType;
    unsigned int vertexCount;
};

enum class Status {
    NOT_GENERATED = 0,
    VOXEL_GENERATED = 1,
    MESH_GENERATED = 2,
    BUFFERS_SETUP = 3
};

class Chunk {
private:
    static unsigned int m_size;
    int m_x, m_y, m_z;
    std::vector<BlockVertex> m_vertices;
    std::vector<BlockFaceData> m_blockFaceData;
    bool m_blockPresent[18][18][18] = {{{false}}}; // 16x16x16 chunk size + 2 for boundary checks
    std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> m_adjacentChunks;
    BlockType m_blockType[18][18][18] = {{{BlockType::AIR}}}; // Default block type
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

    [[nodiscard]] const VertexArray & m_vao() const;
    [[nodiscard]] const IndexBuffer & m_ibo() const;
    [[nodiscard]] static unsigned int m_size1() ;
    [[nodiscard]] const AABB & m_box1() const;
    [[nodiscard]] int m_x_start() const;
    [[nodiscard]] int m_y_start() const;
    [[nodiscard]] int m_z_start() const;
    [[nodiscard]] Status m_status1() const;

private:
    void addBlockFaces(int localX, int localY, int localZ, BlockType blockType);
    bool shouldDrawFace(int localX, int localY, int localZ, bool currentTransparent) const;
    BlockType getBlockType(int localX, int localY, int localZ) const;
    [[nodiscard]] bool isBlockPresent(int localX, int localY, int localZ) const;
};

#endif //CHUNK_H
