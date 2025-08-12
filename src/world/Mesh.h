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
    NEED_BUFFERS_UPDATE,
};

class Mesh {
protected:
    const unsigned int m_size;
    int m_x, m_y, m_z;
    std::vector<BlockVertex> m_vertices;
    std::vector<BlockVertex> m_vertices_transparent;
    std::vector<BlockVertex> m_vertices_water;
    std::vector<BlockFaceData> m_blockFaceData;
    std::vector<BlockFaceData> m_blockFaceData_transparent;
    std::vector<BlockFaceData> m_blockFaceData_water;
    std::vector<BlockType> m_blockType;
    State m_state = State::UNLOADED;
    VertexArray m_VAO;
    VertexArray m_VAO_transparent;
    VertexArray m_VAO_water;
    VertexBuffer m_VBO;
    VertexBuffer m_VBO_transparent;
    VertexBuffer m_VBO_water;
    IndexBuffer m_IBO;
    IndexBuffer m_IBO_transparent;
    IndexBuffer m_IBO_water;
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
        constexpr int VERTEX_COUNT = 4;
        constexpr int NUMBER_OF_FACES = 6;
        // === OPAQUE ===
        if (!m_vertices.empty()) {
            std::vector<unsigned int> meshIndices_opaque;
            meshIndices_opaque.reserve(m_blockFaceData.size() * NUMBER_OF_FACES);
            unsigned int vertexOffsetOpaque = 0;

            for (const auto &[faceType, x, y, z]: m_blockFaceData) {
                constexpr unsigned int faceIndicesCCW[6] = {0, 2, 1, 0, 3, 2};
                constexpr unsigned int faceIndicesCW[6] = {0, 1, 2, 0, 2, 3};
                const unsigned int *indices = faceType == Face::BACK || faceType == Face::LEFT || faceType == Face::TOP
                                                  ? faceIndicesCW : faceIndicesCCW;

                for (int i = 0; i < NUMBER_OF_FACES; ++i) {
                    meshIndices_opaque.push_back(vertexOffsetOpaque + indices[i]);
                }
                vertexOffsetOpaque += VERTEX_COUNT;
            }

            m_VBO.init(m_vertices.data(), m_vertices.size() * sizeof(BlockVertex));
            m_IBO.init(meshIndices_opaque.data(), meshIndices_opaque.size());

            VertexBufferLayout meshLayout;
            meshLayout.PushInt<unsigned char>(3); // x, y, z
            meshLayout.PushInt<unsigned char>(2, true); // u, v
            meshLayout.PushInt<unsigned char>(1); // face

            m_VAO.init();
            m_VAO.addBuffer(m_VBO, meshLayout);
        }

        // === TRANSPARENT ===
        if (!m_vertices_transparent.empty()) {
            std::vector<unsigned int> meshIndices_transparent;
            meshIndices_transparent.reserve(m_blockFaceData_transparent.size() * NUMBER_OF_FACES); // 6 indices par face
            unsigned int vertexOffsetTransparent = 0;

            for (const auto &[faceType, x, y, z]: m_blockFaceData_transparent) {
                constexpr unsigned int faceIndicesCCW[6] = {0, 2, 1, 0, 3, 2};
                constexpr unsigned int faceIndicesCW[6] = {0, 1, 2, 0, 2, 3};
                const unsigned int *indices = faceType == Face::BACK || faceType == Face::LEFT || faceType == Face::TOP
                                                  ? faceIndicesCW : faceIndicesCCW;

                for (int i = 0; i < NUMBER_OF_FACES; ++i) {
                    meshIndices_transparent.push_back(vertexOffsetTransparent + indices[i]);
                }
                vertexOffsetTransparent += VERTEX_COUNT;
            }

            m_VBO_transparent.init(m_vertices_transparent.data(), m_vertices_transparent.size() * sizeof(BlockVertex));
            m_IBO_transparent.init(meshIndices_transparent.data(), meshIndices_transparent.size());

            VertexBufferLayout meshLayout;
            meshLayout.PushInt<unsigned char>(3); // x, y, z
            meshLayout.PushInt<unsigned char>(2, true); // u, v
            meshLayout.PushInt<unsigned char>(1); // face

            m_VAO_transparent.init();
            m_VAO_transparent.addBuffer(m_VBO_transparent, meshLayout);
        }

        // === WATER ===
        if (!m_vertices_water.empty()) {
            std::vector<unsigned int> meshIndices_water;
            meshIndices_water.reserve(m_blockFaceData_water.size() * NUMBER_OF_FACES); // 6 indices par face
            unsigned int vertexOffsetWater = 0;

            for (const auto &[faceType, x, y, z]: m_blockFaceData_water) {
                constexpr unsigned int faceIndicesCCW[6] = {0, 2, 1, 0, 3, 2};
                constexpr unsigned int faceIndicesCW[6] = {0, 1, 2, 0, 2, 3};
                const unsigned int *indices = faceType == Face::BACK || faceType == Face::LEFT || faceType == Face::TOP
                                                  ? faceIndicesCW : faceIndicesCCW;

                for (int i = 0; i < NUMBER_OF_FACES; ++i) {
                    meshIndices_water.push_back(vertexOffsetWater + indices[i]);
                }
                vertexOffsetWater += VERTEX_COUNT;
            }

            m_VBO_water.init(m_vertices_water.data(), m_vertices_water.size() * sizeof(BlockVertex));
            m_IBO_water.init(meshIndices_water.data(), meshIndices_water.size());

            VertexBufferLayout meshLayout;
            meshLayout.PushInt<unsigned char>(3); // x, y, z
            meshLayout.PushInt<unsigned char>(2, true); // u, v
            meshLayout.PushInt<unsigned char>(1); // face

            m_VAO_water.init();
            m_VAO_water.addBuffer(m_VBO_water, meshLayout);
        }

        m_state = State::READY_TO_DRAW;
    }

    void resetGLBuffers() {
        m_VAO.deleteBuffer();
        m_VAO_transparent.deleteBuffer();
        m_VAO_water.deleteBuffer();
        m_VBO.deleteBuffer();
        m_VBO_transparent.deleteBuffer();
        m_VBO_water.deleteBuffer();
        m_IBO.deleteBuffer();
        m_IBO_transparent.deleteBuffer();
        m_IBO_water.deleteBuffer();
    }

    void resetMesh() {
        m_vertices.clear();
        m_vertices_transparent.clear();
        m_vertices_water.clear();
        m_blockFaceData.clear();
        m_blockFaceData_transparent.clear();
        m_blockFaceData_water.clear();
    }

    void draw() const {
        Renderer::draw(m_VAO, m_IBO);
    }

    void drawTransparent() const {
        Renderer::draw(m_VAO_transparent, m_IBO_transparent);
    }

    void drawWater() const {
        Renderer::draw(m_VAO_water, m_IBO_water);
    }

    [[nodiscard]] bool hasOpaqueFaces() const {
        return !m_vertices.empty() && m_IBO.m_count() > 0;
    }

    [[nodiscard]] bool hasTransparentFaces() const {
        return !m_vertices_transparent.empty() && m_IBO_transparent.m_count() > 0;
    }

    [[nodiscard]] bool hasWaterFaces() const {
        return !m_vertices_water.empty() && m_IBO_water.m_count() > 0;
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

    [[nodiscard]] int m_x1() const {
        return m_x;
    }

    [[nodiscard]] int m_y1() const {
        return m_y;
    }

    [[nodiscard]] int m_z1() const {
        return m_z;
    }

    [[nodiscard]] State m_state1() const {
        return m_state;
    }

    [[nodiscard]] const AABB &m_box1() const {
        return m_box;
    }

    [[nodiscard]] const std::vector<BlockVertex> & m_vertices1() const {
        return m_vertices;
    }

    [[nodiscard]] std::vector<BlockFaceData> m_block_face_data() const {
        return m_blockFaceData;
    }
};


#endif //MESH_H
