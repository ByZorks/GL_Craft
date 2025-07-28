#ifndef MESH_H
#define MESH_H

#include <cstdint>

#include "Block.h"
#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"
#include "../math/AABB.h"

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
    std::vector<BlockFaceData> m_blockFaceData;
    std::vector<BlockType> m_blockType;
    Status m_status = Status::NOT_GENERATED;
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    IndexBuffer m_IBO;
    AABB m_box;

public:
    static constexpr unsigned int SIZE = 16;

    Mesh(const int x, const int y, const int z) : m_x(x), m_y(y), m_z(z),
                                                  m_box(AABB(static_cast<float>(x), static_cast<float>(y),
                                                             static_cast<float>(z),
                                                             static_cast<float>(x) + static_cast<float>(Mesh::SIZE) - 1,
                                                             static_cast<float>(y) + static_cast<float>(Mesh::SIZE) - 1,
                                                             static_cast<float>(z) + static_cast<float>(Mesh::SIZE) - 1
                                                  )) {
    }

    virtual ~Mesh() {
        m_vertices.clear();
        m_blockFaceData.clear();
        m_blockType.clear();
    }

    virtual void generateVoxel();
    virtual void generateMesh();
    virtual void setupBuffers();

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

    [[nodiscard]] const VertexArray &m_vao() const {
        return m_VAO;
    }

    [[nodiscard]] const IndexBuffer &m_ibo() const {
        return m_IBO;
    }

    [[nodiscard]] const AABB & m_box1() const {
        return m_box;
    }
};


#endif //MESH_H
