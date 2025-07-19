#ifndef CHUNK_H
#define CHUNK_H
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
    GENERATED = 1,
    MESH_GENERATED = 2,
    BUFFERS_SETUP = 3
};

class Chunk {
private:
    static unsigned int m_size;
    int m_xStart, m_yStart, m_zStart;
    std::vector<float> m_vertices;
    std::vector<BlockFaceData> m_blockFaceData;
    bool m_blockPresent[16][16][16] = {{{false}}}; // 16x16x16 chunk size
    std::unordered_map<std::tuple<int, int, int>, const Chunk*> m_adjacentChunks;
    BlockType m_blockType[16][16][16] = {{{BlockType::STONE}}}; // Default block type
    Status m_status = Status::NOT_GENERATED;
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    IndexBuffer m_IBO;
    AABB m_box;

public:
    Chunk(int x, int y, int z);
    ~Chunk();

    void generateVoxelData(const FastNoiseLite& noiseGenerator);
    void generateMeshData(const World &world);
    void setupBuffers();

    [[nodiscard]] const VertexArray & m_vao() const;
    [[nodiscard]] const IndexBuffer & m_ibo() const;
    [[nodiscard]] static unsigned int m_size1() ;
    [[nodiscard]] const AABB & m_box1() const;
    [[nodiscard]] int m_x_start() const;
    [[nodiscard]] int m_y_start() const;
    [[nodiscard]] int m_z_start() const;
    [[nodiscard]] bool isBlockPresentInLocal(int localX, int localY, int localZ) const;
    [[nodiscard]] Status m_status1() const;

private:
    void addBlockFaces(float worldX, float worldY, float worldZ, BlockType blockType, const World &world);

    bool shouldDrawFace(float nx, float ny, float nz, bool currentTransparent, const World &world);
    BlockType getBlockTypeAt(float worldX, float worldY, float worldZ, const World &world);
    BlockType getBlockTypeAtLocal(int localX, int localY, int localZ) const;

    bool isBlockPresentInWorld(float worldX, float worldY, float worldZ, const World &world);
    bool isBlockPresentInAnotherChunk(float worldX, float worldY, float worldZ, const World &world);
};

#endif //CHUNK_H
