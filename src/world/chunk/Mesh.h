#ifndef MESH_H
#define MESH_H

#include "../Block.h"
#include "../../gl/VertexArray.h"
#include "../../math/AABB.h"

struct MeshingResult;

class Mesh {
public:
    enum class State : uint8_t {
        UNINITIALIZED,
        VOXEL_GENERATED,
        READY_TO_DRAW,
    };

public:
    Mesh(int x, int y, int z, unsigned int size);

    virtual ~Mesh() = default;

    virtual void generateVoxel();
    virtual void generateMesh();

    void updateVertexCount();
    void resetMesh();

    [[nodiscard]] bool isEmpty() const;

    [[nodiscard]] bool hasOpaqueFaces() const;
    void setHasOpaqueFaces(bool hasFaces);
    [[nodiscard]] bool hasWaterFaces() const;
    void setHasWaterFaces(bool hasFaces);

    [[nodiscard]] bool shouldDrawFace(int x, int y, int z, Block::BlockType currentBlockType, Block::Face face) const;
    [[nodiscard]] virtual bool isBlockPresent(int localX, int localY, int localZ) const;
    [[nodiscard]] virtual Block::BlockType getBlockType(int localX, int localY, int localZ) const;
    [[nodiscard]] virtual int index(int x, int y, int z) const;

    [[nodiscard]] int getX() const;
    [[nodiscard]] int getY() const;
    [[nodiscard]] int getZ() const;

    [[nodiscard]] State getState() const;
    void setState(State m_state);
    [[nodiscard]] const AABB &getBoundingBox() const;
    void setWasInFrustum(bool isInFrustum);
    [[nodiscard]] bool wasInFrustum() const;

    [[nodiscard]] std::vector<Block::BlockVertex> getOpaqueVerticesCopy() const;
    [[nodiscard]] const std::vector<Block::BlockVertex> &getOpaqueVertices() const;
    [[nodiscard]] std::vector<Block::BlockVertex> &getOpaqueVertices();
    [[nodiscard]] const std::vector<Block::BlockVertex> &getWaterVertices() const;
    [[nodiscard]] std::vector<Block::BlockVertex> &getWaterVertices();
    [[nodiscard]] unsigned int getOpaqueVertexCount() const;
    [[nodiscard]] unsigned int getWaterVertexCount() const;

protected:
    struct BufferData {
        std::vector<Block::BlockVertex> vertices;
        unsigned int verticesCount = 0; // vertices.size() * 6; Used to draw the mesh, so we must only update it once the GL buffers are ready
        bool hasFaces = false;

        void shrinkVertices() {
            vertices.shrink_to_fit();
        }

        void deleteMesh() {
            vertices.clear();
        }
    };

    const unsigned int m_size;
    const int m_x, m_y, m_z;
    std::vector<Block::BlockType> m_blocks;
    std::vector<uint8_t> m_lightLevels;
    BufferData m_opaqueData;
    BufferData m_waterData;
    State m_state = State::UNINITIALIZED;
    const AABB m_box;
    bool m_wasInFrustum = false;
    unsigned int m_visibleBlocks = 0;
};

#endif //MESH_H
