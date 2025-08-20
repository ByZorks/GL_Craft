#ifndef MESH_H
#define MESH_H

#include <mutex>

#include "Block.h"
#include "../gl/StorageBuffer.h"
#include "../gl/VertexArray.h"
#include "../math/AABB.h"
#include "../render/Renderer.h"

enum class State : uint8_t {
    UNINITIALIZED,
    VOXEL_GENERATED,
    MESH_GENERATED,
    READY_TO_DRAW,
};

struct GLBuffersData {
    mutable std::mutex m_verticesMutex;
    std::vector<BlockVertex> vertices;
    unsigned int verticesSize = 0; // vertices.size(); Used to draw the mesh, so we must only update it once the GL buffers are ready
    StorageBuffer SSBO;
    bool hasFaces = true;

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
    VertexArray VAO; // Not really used as we are using vertex pulling, only one needed to issue draw calls
    GLBuffersData m_opaqueData;
    GLBuffersData m_transparentData;
    GLBuffersData m_waterData;
    State m_state = State::UNINITIALIZED;
    const AABB m_box;

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
    virtual void generateMesh(bool setFlag);

    void createGLBuffers() {
        VAO.init();

        std::scoped_lock lock(m_opaqueData.m_verticesMutex,
                      m_transparentData.m_verticesMutex,
                      m_waterData.m_verticesMutex);

        if (!m_opaqueData.vertices.empty()) {
            setupGLBuffers(m_opaqueData.vertices, m_opaqueData.SSBO);
        } else {
            m_opaqueData.hasFaces = false;
        }
        if (!m_transparentData.vertices.empty()) {
            setupGLBuffers(m_transparentData.vertices, m_transparentData.SSBO);
        } else {
            m_transparentData.hasFaces = false;
        }
        if (!m_waterData.vertices.empty()) {
            setupGLBuffers(m_waterData.vertices, m_waterData.SSBO);
        } else {
            m_waterData.hasFaces = false;
        }

        m_opaqueData.verticesSize = static_cast<unsigned int>(m_opaqueData.vertices.size());
        m_transparentData.verticesSize = static_cast<unsigned int>(m_transparentData.vertices.size());
        m_waterData.verticesSize = static_cast<unsigned int>(m_waterData.vertices.size());

        m_state = State::READY_TO_DRAW;
    }

    void updateGLBuffers() {
        VAO.init();

        std::scoped_lock lock(m_opaqueData.m_verticesMutex,
                      m_transparentData.m_verticesMutex,
                      m_waterData.m_verticesMutex);

        // == OPAQUE ==
        if (!m_opaqueData.vertices.empty()) {
            if (m_opaqueData.SSBO.getBindingPoint() != 0) {
                setupGLBuffers(m_opaqueData.vertices, m_opaqueData.SSBO);
            } else {
                m_opaqueData.SSBO.updateData(m_opaqueData.vertices.data(), static_cast<unsigned int>(m_opaqueData.vertices.size() * sizeof(BlockVertex)));
            }
            m_opaqueData.hasFaces = true;
        } else {
            m_opaqueData.hasFaces = false;
        }

        // == TRANSPARENT ==
        if (!m_transparentData.vertices.empty()) {
            if (m_transparentData.SSBO.getBindingPoint() != 0) {
                setupGLBuffers(m_transparentData.vertices, m_transparentData.SSBO);
            } else {
                m_transparentData.SSBO.updateData(m_transparentData.vertices.data(), static_cast<unsigned int>(m_transparentData.vertices.size() * sizeof(BlockVertex)));
            }
            m_transparentData.hasFaces = true;
        } else {
            m_transparentData.hasFaces = false;
        }

        // == WATER ==
        if (!m_waterData.vertices.empty()) {
            if (m_waterData.SSBO.getBindingPoint() != 0) {
                setupGLBuffers(m_waterData.vertices, m_waterData.SSBO);
            } else {
                m_waterData.SSBO.updateData(m_waterData.vertices.data(), static_cast<unsigned int>(m_waterData.vertices.size() * sizeof(BlockVertex)));
            }
            m_waterData.hasFaces = true;
        } else {
            m_waterData.hasFaces = false;
        }

        m_opaqueData.verticesSize = static_cast<unsigned int>(m_opaqueData.vertices.size());
        m_transparentData.verticesSize = static_cast<unsigned int>(m_transparentData.vertices.size());
        m_waterData.verticesSize = static_cast<unsigned int>(m_waterData.vertices.size());

        m_state = State::READY_TO_DRAW;
    }

    void resetMesh() {
        m_opaqueData.deleteMesh();
        m_transparentData.deleteMesh();
        m_waterData.deleteMesh();
    }

    void draw() const {
        Renderer::drawWithVertexPulling(VAO, m_opaqueData.SSBO, m_opaqueData.verticesSize * 6); // 6 vertices per face
    }

    void drawTransparent() const {
        Renderer::drawWithVertexPulling(VAO, m_transparentData.SSBO, m_transparentData.verticesSize * 6);
    }

    void drawWater() const {
        Renderer::drawWithVertexPulling(VAO, m_waterData.SSBO, m_waterData.verticesSize* 6);
    }

    [[nodiscard]] bool hasOpaqueFaces() const {
        return m_opaqueData.hasFaces;
    }

    [[nodiscard]] bool hasTransparentFaces() const {
        return m_transparentData.hasFaces;
    }

    [[nodiscard]] bool hasWaterFaces() const {
        return m_waterData.hasFaces;
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

    [[nodiscard]] const AABB &getBoundingBox() const {
        return m_box;
    }

    [[nodiscard]] std::vector<BlockVertex> getOpaqueVerticesCopy() const {
        std::lock_guard lock(m_opaqueData.m_verticesMutex);
        return m_opaqueData.vertices; // Only used for instance rendering, so a copy is fine
    }

private:
    static void setupGLBuffers(const std::vector<BlockVertex> &vertices, StorageBuffer &SSBO) {
        SSBO.init(vertices.data(), static_cast<unsigned int>(vertices.size() * sizeof(BlockVertex)), 1);
    }
};


#endif //MESH_H
