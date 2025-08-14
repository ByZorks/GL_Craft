#ifndef MESH_H
#define MESH_H

#include "Block.h"
#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"
#include "../math/AABB.h"
#include "../render/Renderer.h"

enum class State : uint8_t {
    UNLOADED,
    VOXEL_GENERATED,
    MESH_GENERATED,
    READY_TO_DRAW,
};

struct GLBuffersData {
    std::vector<BlockVertex> vertices;
    std::vector<BlockFaceData> blockFaceData;
    VertexArray VAO;
    VertexBuffer VBO;
    IndexBuffer IBO;

    void shrinkBuffers() {
        vertices.shrink_to_fit();
        blockFaceData.shrink_to_fit();
    }

    void deleteMesh() {
        vertices.clear();
        blockFaceData.clear();
    }

    void deleteGLBuffer() {
        VAO.deleteBuffer();
        VBO.deleteBuffer();
        IBO.deleteBuffer();
    }
};

class Mesh {
protected:
    const unsigned int m_size;
    int m_x, m_y, m_z;
    GLBuffersData m_opaqueData;
    GLBuffersData m_transparentData;
    GLBuffersData m_waterData;
    std::vector<BlockType> m_blockType;
    State m_state = State::UNLOADED;
    AABB m_box;

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
    virtual void generateMesh();

    void createGLBuffers() {
        if (!m_opaqueData.vertices.empty()) setupGLBuffers(m_opaqueData.vertices, m_opaqueData.blockFaceData, m_opaqueData.VAO, m_opaqueData.VBO, m_opaqueData.IBO);
        if (!m_transparentData.vertices.empty()) setupGLBuffers(m_transparentData.vertices, m_transparentData.blockFaceData, m_transparentData.VAO, m_transparentData.VBO, m_transparentData.IBO);
        if (!m_waterData.vertices.empty()) setupGLBuffers(m_waterData.vertices, m_waterData.blockFaceData, m_waterData.VAO, m_waterData.VBO, m_waterData.IBO);

        m_state = State::READY_TO_DRAW;
    }

    void resetGLBuffers() {
        m_opaqueData.deleteGLBuffer();
        m_transparentData.deleteGLBuffer();
        m_waterData.deleteGLBuffer();
    }

    void resetMesh() {
        m_opaqueData.deleteMesh();
        m_transparentData.deleteMesh();
        m_waterData.deleteMesh();
    }

    void draw() const {
        Renderer::draw(m_opaqueData.VAO, m_opaqueData.IBO);
    }

    void drawTransparent() const {
        Renderer::draw(m_transparentData.VAO, m_transparentData.IBO);
    }

    void drawWater() const {
        Renderer::draw(m_waterData.VAO, m_waterData.IBO);
    }

    [[nodiscard]] bool hasOpaqueFaces() const {
        return !m_opaqueData.vertices.empty() && m_opaqueData.IBO.getCount() > 0;
    }

    [[nodiscard]] bool hasTransparentFaces() const {
        return !m_transparentData.vertices.empty() && m_transparentData.IBO.getCount() > 0;
    }

    [[nodiscard]] bool hasWaterFaces() const {
        return !m_waterData.vertices.empty() && m_waterData.IBO.getCount() > 0;
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

    [[nodiscard]] std::vector<BlockFaceData> getOpaqueBlockFaceData() const {
        return m_opaqueData.blockFaceData;
    }

private:
    static void setupGLBuffers(const std::vector<BlockVertex> &vertices, std::vector<BlockFaceData> &blockFaceData,
                      VertexArray &VAO, VertexBuffer &VBO, IndexBuffer &IBO) {
        constexpr int NUMBER_OF_FACES = 6;
        std::vector<unsigned int> indices;
        indices.reserve(blockFaceData.size() * NUMBER_OF_FACES);
        unsigned int vertexOffsetOpaque = 0;

        for (const auto &[faceType, x, y, z]: blockFaceData) {
            constexpr unsigned int faceIndicesCCW[6] = {0, 2, 1, 0, 3, 2};
            constexpr unsigned int faceIndicesCW[6] = {0, 1, 2, 0, 2, 3};
            const unsigned int *indicesOrder = faceType == Face::BACK || faceType == Face::LEFT || faceType == Face::TOP
                                              ? faceIndicesCW : faceIndicesCCW;

            for (int i = 0; i < NUMBER_OF_FACES; ++i) {
                indices.push_back(vertexOffsetOpaque + indicesOrder[i]);
            }

            constexpr int VERTEX_COUNT = 4;
            vertexOffsetOpaque += VERTEX_COUNT;
        }

        VBO.init(vertices.data(), vertices.size() * sizeof(BlockVertex));
        IBO.init(indices.data(), indices.size());

        VertexBufferLayout meshLayout;
        meshLayout.PushInt<unsigned char>(3); // x, y, z
        meshLayout.PushInt<unsigned char>(2, true); // u, v
        meshLayout.PushInt<unsigned char>(1); // face
        meshLayout.PushInt<unsigned char>(1); // AO

        VAO.init();
        VAO.addBuffer(VBO, meshLayout);
    }
};


#endif //MESH_H
