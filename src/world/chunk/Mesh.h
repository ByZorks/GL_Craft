#ifndef MESH_H
#define MESH_H

#include <mutex>

#include "../Block.h"
#include "../../gl/VertexArray.h"
#include "../../math/AABB.h"

struct MeshingResult;

enum class State : uint8_t {
    UNINITIALIZED,
    VOXEL_GENERATED,
    READY_TO_DRAW,
};

struct buffersData {
    std::vector<BlockVertex> vertices;
    unsigned int verticesCount = 0; // vertices.size() * 6; Used to draw the mesh, so we must only update it once the GL buffers are ready
    bool hasFaces = false;

    void shrinkVertices() {
        vertices.shrink_to_fit();
    }

    void deleteMesh() {
        vertices.clear();
    }
};

class Mesh {
protected:
    const unsigned int m_size;
    const int m_x, m_y, m_z;
    std::vector<BlockType> m_blocks;
    buffersData m_opaqueData;
    buffersData m_waterData;
    State m_state = State::UNINITIALIZED;
    const AABB m_box;
    bool m_wasInFrustum = false;
    unsigned int m_visibleBlocks = 0;

public:
    Mesh(int x, int y, int z, unsigned int size);

    virtual ~Mesh() = default;

    virtual void generateVoxel();
    virtual void generateMesh();

    void updateVertexCount();
    void resetMesh();

    bool isEmpty() const;

    [[nodiscard]] bool hasOpaqueFaces() const;
    void setHasOpaqueFaces(bool hasFaces);

    [[nodiscard]] bool hasWaterFaces() const;
    void setHasWaterFaces(bool hasFaces);

    [[nodiscard]] bool shouldDrawFace(int x, int y, int z, BlockType currentBlockType, Face face) const;
    [[nodiscard]] virtual bool isBlockPresent(int localX, int localY, int localZ) const;
    [[nodiscard]] virtual BlockType getBlockType(int localX, int localY, int localZ) const;
    [[nodiscard]] virtual int index(int x, int y, int z) const;

    [[nodiscard]] int getX() const;
    [[nodiscard]] int getY() const;
    [[nodiscard]] int getZ() const;

    [[nodiscard]] State getState() const;
    void setState(State m_state);
    [[nodiscard]] const AABB &getBoundingBox() const;
    void setWasInFrustum(bool isInFrustum);
    [[nodiscard]] bool wasInFrustum() const;

    [[nodiscard]] std::vector<BlockVertex> getOpaqueVerticesCopy() const;
    [[nodiscard]] const std::vector<BlockVertex> &getOpaqueVertices() const;
    [[nodiscard]] std::vector<BlockVertex> &getOpaqueVertices();
    [[nodiscard]] const std::vector<BlockVertex> &getWaterVertices() const;
    [[nodiscard]] std::vector<BlockVertex> &getWaterVertices();
    [[nodiscard]] unsigned int getOpaqueVertexCount() const;
    [[nodiscard]] unsigned int getWaterVertexCount() const;
};


#endif //MESH_H
