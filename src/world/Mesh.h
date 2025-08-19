#ifndef MESH_H
#define MESH_H

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
    std::vector<BlockVertex> vertices;
    VertexArray VAO;
    StorageBuffer SSBO;
    bool hasFaces = true;

    void shrinkBuffers() {
        vertices.shrink_to_fit();
    }

    void deleteMesh() {
        vertices.clear();
    }

    void deleteGLBuffer() {
        VAO.deleteBuffer();
    }
};

class Mesh {
protected:
    const unsigned int m_size;
    const int m_x, m_y, m_z;
    std::vector<BlockType> m_blockType;
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
        m_blockType.resize(m_size * m_size * m_size, BlockType::AIR);
    }
    virtual ~Mesh() = default;

    virtual void generateVoxel();
    virtual void generateMesh(bool setFlag);

    void createGLBuffers() {
        if (!m_opaqueData.vertices.empty()) {
            setupGLBuffers(m_opaqueData.vertices, m_opaqueData.VAO, m_opaqueData.SSBO);
        } else {
            m_opaqueData.hasFaces = false;
        }
        if (!m_transparentData.vertices.empty()) {
            setupGLBuffers(m_transparentData.vertices, m_transparentData.VAO, m_transparentData.SSBO);
        } else {
            m_transparentData.hasFaces = false;
        }
        if (!m_waterData.vertices.empty()) {
            setupGLBuffers(m_waterData.vertices, m_waterData.VAO, m_waterData.SSBO);
        } else {
            m_waterData.hasFaces = false;
        }

        m_state = State::READY_TO_DRAW;
    }

    void createNewMeshGLBuffers() {
        bool hasNewOpaque = false, hasNewTransparent = false, hasNewWater = false;
        StorageBuffer newOpaqueSSBO, newTransparentSSBO, newWaterSSBO;

        // Create new GL buffers for the temporary mesh data
        if (!m_opaqueData.vertices.empty()) {
            setupGLBuffers(m_opaqueData.vertices, m_opaqueData.VAO,newOpaqueSSBO);
            hasNewOpaque = true;
        }
        if (!m_transparentData.vertices.empty()) {
            setupGLBuffers(m_transparentData.vertices, m_transparentData.VAO, newTransparentSSBO);
            hasNewTransparent = true;
        }
        if (!m_waterData.vertices.empty()) {
            setupGLBuffers(m_waterData.vertices, m_waterData.VAO, newWaterSSBO);
            hasNewWater = true;
        }

        // Switch the buffers
        if (hasNewOpaque) {
            m_opaqueData.SSBO = std::move(newOpaqueSSBO);
            m_opaqueData.hasFaces = true;
        }
        if (hasNewTransparent) {
            m_transparentData.SSBO = std::move(newTransparentSSBO);
            m_transparentData.hasFaces = true;
        }
        if (hasNewWater) {
            m_waterData.SSBO = std::move(newWaterSSBO);
            m_waterData.hasFaces = true;
        }

        m_state = State::READY_TO_DRAW;
    }

    void resetGLBuffers() {
        m_opaqueData.deleteGLBuffer();
        m_transparentData.deleteGLBuffer();
        m_waterData.deleteGLBuffer();

        m_state = State::MESH_GENERATED;
    }

    void resetMesh() {
        m_opaqueData.deleteMesh();
        m_transparentData.deleteMesh();
        m_waterData.deleteMesh();
    }

    void draw() const {
        Renderer::drawWithVertexPulling(m_opaqueData.VAO, m_opaqueData.SSBO, m_opaqueData.vertices.size() * 6); // 6 vertices per face
    }

    void drawTransparent() const {
        Renderer::drawWithVertexPulling(m_transparentData.VAO, m_transparentData.SSBO, m_transparentData.vertices.size() * 6);
    }

    void drawWater() const {
        Renderer::drawWithVertexPulling(m_waterData.VAO, m_waterData.SSBO, m_waterData.vertices.size() * 6);
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

    [[nodiscard]] const std::vector<BlockVertex> & getOpaqueVertices() const {
        return m_opaqueData.vertices;
    }

private:
    static void setupGLBuffers(const std::vector<BlockVertex> &vertices, VertexArray &VAO, StorageBuffer &SSBO) {
        VAO.init();
        SSBO.init(vertices.data(), static_cast<unsigned int>(vertices.size() * sizeof(BlockVertex)), 1);
    }
};


#endif //MESH_H
