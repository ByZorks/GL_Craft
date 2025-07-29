#ifndef MESH_H
#define MESH_H

#include <cstdint>

#include "Block.h"
#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"
#include "../math/AABB.h"
#include "../render/Renderer.h"

enum class Status : uint8_t {
    NOT_GENERATED,
    VOXEL_GENERATED,
    MESH_GENERATED,
    BUFFERS_SETUP
};

class Mesh {
protected:
    int m_x, m_y, m_z;
    std::vector<BlockVertex> m_vertices;
    std::vector<BlockFaceData> m_blockFaceDataOpaque;
    std::vector<BlockFaceData> m_blockFaceDataTransparent;
    std::vector<BlockType> m_blockType;
    Status m_status = Status::NOT_GENERATED;
    VertexArray m_VAO_opaque;
    VertexArray m_VAO_transparent;
    VertexBuffer m_VBO;
    IndexBuffer m_IBO_opaque;
    IndexBuffer m_IBO_transparent;
    AABB m_box;

public:
    static constexpr unsigned int SIZE = 16;

    Mesh(const int x, const int y, const int z) : m_x(x), m_y(y), m_z(z),
                                                  m_box(AABB(static_cast<float>(x), static_cast<float>(y),
                                                             static_cast<float>(z),
                                                             static_cast<float>(x) + static_cast<float>(SIZE) - 1,
                                                             static_cast<float>(y) + static_cast<float>(SIZE) - 1,
                                                             static_cast<float>(z) + static_cast<float>(SIZE) - 1
                                                  )) {
    }

    virtual ~Mesh() {
        m_vertices.clear();
        m_blockFaceDataOpaque.clear();
        m_blockType.clear();
    }

    virtual void generateVoxel();
    virtual void generateMesh();

    void setupBuffers() {
        std::vector<unsigned int> meshIndices_opaque;
        meshIndices_opaque.reserve(m_vertices.size() * 6); // 6 indices per face
        unsigned int vertexOffset = 0;

        for (const auto &[faceType, vertexCount, x, y, z]: m_blockFaceDataOpaque) {
            constexpr unsigned int faceIndicesCCW[6] = {0, 2, 1, 0, 3, 2};
            constexpr unsigned int faceIndicesCW[6] = {0, 1, 2, 0, 2, 3};
            const unsigned int *indices = faceType == Face::BACK || faceType == Face::LEFT || faceType == Face::TOP
                                              ? faceIndicesCW
                                              : faceIndicesCCW;
            for (int i = 0; i < 6; ++i) {
                unsigned int index = vertexOffset + indices[i];
                meshIndices_opaque.push_back(index);
            }

            vertexOffset += vertexCount;
        }

        std::vector<unsigned int> meshIndices_transparent;
        meshIndices_transparent.reserve(m_vertices.size() * 6); // 6 indices per face
        for (const auto &[faceType, vertexCount, x, y, z]: m_blockFaceDataTransparent) {
            constexpr unsigned int faceIndicesCCW[6] = {0, 2, 1, 0, 3, 2};
            constexpr unsigned int faceIndicesCW[6] = {0, 1, 2, 0, 2, 3};
            const unsigned int *indices = faceType == Face::BACK || faceType == Face::LEFT || faceType == Face::TOP
                                              ? faceIndicesCW
                                              : faceIndicesCCW;
            for (int i = 0; i < 6; ++i) {
                unsigned int index = vertexOffset + indices[i];
                meshIndices_transparent.push_back(index);
            }

            vertexOffset += vertexCount;
        }

        m_VBO.init(m_vertices.data(), m_vertices.size() * sizeof(BlockVertex));
        m_IBO_opaque.init(meshIndices_opaque.data(), meshIndices_opaque.size());
        m_IBO_transparent.init(nullptr, meshIndices_transparent.size());

        VertexBufferLayout meshLayout;
        meshLayout.Push<unsigned char>(3); // x, y, z
        meshLayout.Push<unsigned char>(2, true); // u, v
        meshLayout.PushInt<unsigned char>(1); // face
        m_VAO_opaque.init();
        m_VAO_opaque.AddBuffer(m_VBO, meshLayout);
        m_VAO_transparent.init();
        m_VAO_transparent.AddBuffer(m_VBO, meshLayout);

        m_status = Status::BUFFERS_SETUP;
    }

    void updateTransparentIBO() {
        std::vector<unsigned int> meshIndices_transparent;
        meshIndices_transparent.reserve(m_blockFaceDataTransparent.size());
        unsigned int vertexOffset = 0;

        for (const auto &[faceType, vertexCount, x, y, z]: m_blockFaceDataTransparent) {
            constexpr unsigned int faceIndicesCCW[6] = {0, 2, 1, 0, 3, 2};
            constexpr unsigned int faceIndicesCW[6] = {0, 1, 2, 0, 2, 3};
            const unsigned int *indices = faceType == Face::BACK || faceType == Face::LEFT || faceType == Face::TOP
                                              ? faceIndicesCW
                                              : faceIndicesCCW;
            for (int i = 0; i < 6; ++i) {
                unsigned int index = vertexOffset + indices[i];
                meshIndices_transparent.push_back(index);
            }

            vertexOffset += vertexCount;
        }

        m_IBO_transparent.updateData(meshIndices_transparent.data());
    }

    void drawOpaque() const {
        Renderer::draw(m_vao_opaque(), m_ibo_opaque());
    }

    void drawTransparent() const {
        Renderer::draw(m_vao_transparent(), m_ibo_transparent());
    }

    [[nodiscard]] bool hasOpaqueFaces() const {
        return m_IBO_opaque.m_count() > 0;
    }

    [[nodiscard]] bool hasTransparentFaces() const {
        return m_IBO_transparent.m_count() > 0;
    }

    [[nodiscard]] virtual bool shouldDrawFace(const int neighborX, const int neighborY, const int neighborZ,
                                              const BlockType currentBlockType) const {
        if (!isBlockPresent(neighborX, neighborY, neighborZ)) return true; // Air block

        const BlockType neighborType = getBlockType(neighborX, neighborY, neighborZ);
        const bool neighborTransparent = Block::isTransparent(neighborType);

        if (currentBlockType == BlockType::LEAVES && neighborTransparent) return true; // Leaves block, always draw face
        if (currentBlockType == neighborType) return false; // Same block type, no need to draw face

        const bool currentTransparent = Block::isTransparent(currentBlockType);

        return currentTransparent != neighborTransparent; // Different transparency state, draw face
    }

    [[nodiscard]] virtual bool isBlockPresent(const int localX, const int localY, const int localZ) const {
        return m_blockType[index(localX, localY, localZ)] != BlockType::AIR;
    }

    [[nodiscard]] virtual BlockType getBlockType(const int localX, const int localY, const int localZ) const {
        return m_blockType[index(localX, localY, localZ)];
    }

    static int index(const int x, const int y, const int z) {
        constexpr int stride = SIZE;
        return x * stride * stride + y * stride + z;
    }

    [[nodiscard]] int m_x1() const {
        return m_x;
    }

    [[nodiscard]] int m_y1() const {
        return m_y;
    }

    [[nodiscard]] int m_z1() const {
        return m_z;
    }

    [[nodiscard]] Status m_status1() const {
        return m_status;
    }

    [[nodiscard]] const VertexArray &m_vao_opaque() const {
        return m_VAO_opaque;
    }

    [[nodiscard]] const VertexArray & m_vao_transparent() const {
        return m_VAO_transparent;
    }

    [[nodiscard]] const IndexBuffer &m_ibo_opaque() const {
        return m_IBO_opaque;
    }

    [[nodiscard]] const IndexBuffer & m_ibo_transparent() const {
        return m_IBO_transparent;
    }

    [[nodiscard]] const AABB &m_box1() const {
        return m_box;
    }

    [[nodiscard]] std::vector<BlockFaceData> & m_block_face_data_transparent() {
        return m_blockFaceDataTransparent;
    }
};


#endif //MESH_H
