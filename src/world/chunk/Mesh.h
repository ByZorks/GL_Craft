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
    mutable std::mutex m_verticesMutex;
    std::vector<BlockVertex> vertices;
    unsigned int verticesCount = 0; // vertices.size() * 6; Used to draw the mesh, so we must only update it once the GL buffers are ready
    bool hasFaces = false;

    void shrinkBuffers() {
        vertices.shrink_to_fit();
    }

    void deleteMesh() {
        std::lock_guard lock(m_verticesMutex);
        vertices.clear();
    }
};

class Mesh {
protected:
    const unsigned int m_size;
    const int m_x, m_y, m_z;
    std::vector<BlockType> m_blockType;
    buffersData m_opaqueData;
    buffersData m_waterData;
    State m_state = State::UNINITIALIZED;
    const AABB m_box;
    bool m_wasInFrustum = false;

public:
    Mesh(const int x, const int y, const int z, const unsigned int size) : m_size(size), m_x(x), m_y(y), m_z(z),
                                                  m_box(AABB(static_cast<float>(x), static_cast<float>(y),
                                                             static_cast<float>(z),
                                                             static_cast<float>(x) + static_cast<float>(size) - 1.0f,
                                                             static_cast<float>(y) + static_cast<float>(size) - 1.0f,
                                                             static_cast<float>(z) + static_cast<float>(size) - 1.0f
                                                  )) {
        m_blockType.reserve(m_size * m_size * m_size);
        m_blockType.resize(m_size * m_size * m_size, BlockType::AIR);
    }
    virtual ~Mesh() = default;

    virtual void generateVoxel();
    virtual void generateMesh();

    void updateVertexCount() {
        m_opaqueData.verticesCount = static_cast<unsigned int>(m_opaqueData.vertices.size() * 6); // Only 1 vertex is stored
        m_waterData.verticesCount = static_cast<unsigned int>(m_waterData.vertices.size() * 6);
    }

    void resetMesh() {
        m_opaqueData.deleteMesh();
        m_waterData.deleteMesh();
    }

    [[nodiscard]] bool hasOpaqueFaces() const {
        return m_opaqueData.hasFaces;
    }

    void setHasOpaqueFaces(const bool hasFaces) {
        m_opaqueData.hasFaces = hasFaces;
    }

    [[nodiscard]] bool hasWaterFaces() const {
        return m_waterData.hasFaces;
    }

    void setHasWaterFaces(const bool hasFaces) {
        m_waterData.hasFaces = hasFaces;
    }

    [[nodiscard]] virtual bool shouldDrawFace(int x, int y, int z,
                                              const BlockType currentBlockType, const Face face) const {
        switch (face) {
            case Face::TOP: y++; break;
            case Face::BOTTOM: y--; break;
            case Face::FRONT: z++; break;
            case Face::BACK: z--; break;
            case Face::RIGHT: x++; break;
            case Face::LEFT: x--; break;
            default: ;
        }

        if (!isBlockPresent(x, y, z)) return true; // Air block

        const BlockType neighborType = getBlockType(x, y, z);
        const bool neighborTransparent = Block::isTransparent(neighborType);

        if (currentBlockType == BlockType::LEAVES && neighborTransparent) return true; // Leaves block, always draw face
        if (currentBlockType == BlockType::WATER && face == Face::TOP && neighborType != BlockType::WATER) return true; // Always draw water top face if neighbor is not water
        if (currentBlockType == neighborType) return false; // Same block type, no need to draw face

        const bool currentTransparent = Block::isTransparent(currentBlockType);
        if (currentTransparent && !neighborTransparent) return false; // Current block is transparent, neighbor is not, do not draw face

        return currentTransparent != neighborTransparent; // Different transparency state, draw face
    }

    [[nodiscard]] virtual bool isBlockPresent(const int localX, const int localY, const int localZ) const {
        if (localX < 0 || localY < 0 || localZ < 0 || localX >= m_size || localY >= m_size || localZ >= m_size) {
            return false;
        }
        return m_blockType[index(localX, localY, localZ)] != BlockType::AIR;
    }

    [[nodiscard]] virtual BlockType getBlockType(const int localX, const int localY, const int localZ) const {
        if (localX < 0 || localY < 0 || localZ < 0 || localX >= m_size || localY >= m_size || localZ >= m_size) {
            return BlockType::AIR;
        }
        return m_blockType[index(localX, localY, localZ)];
    }

    [[nodiscard]] virtual int index(const int x, const int y, const int z) const {
        const int stride = static_cast<int>(m_size);
        return x * stride * stride + y * stride + z;
    }

    [[nodiscard]] int getX() const {
        return m_x;
    }

    [[nodiscard]] int getY() const {
        return m_y;
    }

    [[nodiscard]] int getZ() const {
        return m_z;
    }

    [[nodiscard]] State getState() const {
        return m_state;
    }

    void setState(const State m_state) {
        this->m_state = m_state;
    }

    [[nodiscard]] const AABB &getBoundingBox() const {
        return m_box;
    }

    void setWasInFrustum(const bool isInFrustum) {
        m_wasInFrustum = isInFrustum;
    }

    [[nodiscard]] bool wasInFrustum() const {
        return m_wasInFrustum;
    }

    [[nodiscard]] std::vector<BlockVertex> getOpaqueVerticesCopy() const {
        std::lock_guard lock(m_opaqueData.m_verticesMutex);
        return m_opaqueData.vertices; // Only used for instance rendering, so a copy is fine
    }

    [[nodiscard]] const std::vector<BlockVertex> & getOpaqueVertices() const {
        return m_opaqueData.vertices;
    }

    [[nodiscard]] std::vector<BlockVertex> & getOpaqueVertices() {
        return m_opaqueData.vertices;
    }

    [[nodiscard]] const std::vector<BlockVertex> & getWaterVertices() const {
        return m_waterData.vertices;
    }

    [[nodiscard]] std::vector<BlockVertex> & getWaterVertices() {
        return m_waterData.vertices;
    }

    [[nodiscard]] unsigned int getOpaqueVertexCount() const {
        return m_opaqueData.verticesCount;
    }

    [[nodiscard]] unsigned int getWaterVertexCount() const {
        return m_waterData.verticesCount;
    }
};


#endif //MESH_H
