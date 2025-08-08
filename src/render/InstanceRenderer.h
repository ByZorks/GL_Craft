#ifndef INSTANCERENDERER_H
#define INSTANCERENDERER_H
#include "vec3.hpp"
#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"
#include "../world/Block.h"

class InstanceRenderer {
private:
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    VertexBuffer m_instanceVBO;
    IndexBuffer m_IBO;

    std::vector<glm::vec3> m_instancePositions;
    unsigned int m_instanceCount = 0;
    size_t m_instanceBufferCapacity = 0;
    bool m_buffersInitialized = false;

public:
    template<typename MeshType>
    void init(const MeshType &mesh) {
        if (m_buffersInitialized) return;

        MeshType meshCopy = mesh;
        meshCopy.generateVoxel();
        meshCopy.generateMesh();

        const auto& vertices = meshCopy.m_vertices1();
        const auto& verticesTransparent = meshCopy.m_vertices_transparent1();
        const auto& blockFaceData = meshCopy.m_block_face_data();
        const auto& blockFaceDataTransparent = meshCopy.m_block_face_data_transparent();

        if (!vertices.empty()) {
            std::vector<unsigned int> meshIndices_opaque;
            meshIndices_opaque.reserve(blockFaceData.size() * 6); // 6 indices per face
            unsigned int vertexOffsetOpaque = 0;

            for (const auto &[faceType, vertexCount, x, y, z]: blockFaceData) {
                constexpr unsigned int faceIndicesCCW[6] = {0, 2, 1, 0, 3, 2};
                constexpr unsigned int faceIndicesCW[6] = {0, 1, 2, 0, 2, 3};
                const unsigned int *indices = faceType == Face::BACK || faceType == Face::LEFT || faceType == Face::TOP
                                                  ? faceIndicesCW : faceIndicesCCW;

                for (int i = 0; i < 6; ++i) {
                    meshIndices_opaque.push_back(vertexOffsetOpaque + indices[i]);
                }
                vertexOffsetOpaque += vertexCount;
            }

            m_VBO.init(vertices.data(), vertices.size() * sizeof(BlockVertex));
            m_IBO.init(meshIndices_opaque.data(), meshIndices_opaque.size());

            VertexBufferLayout meshLayout;
            meshLayout.PushInt<unsigned char>(3); // x, y, z
            meshLayout.PushInt<unsigned char>(2, true); // u, v
            meshLayout.PushInt<unsigned char>(1); // face

            m_VAO.init();
            m_VAO.addBuffer(m_VBO, meshLayout);
        }

        // Instance buffer
        constexpr size_t initialCapacity = 10000;
        m_instancePositions.reserve(initialCapacity);
        m_instanceVBO.init(nullptr, initialCapacity * sizeof(glm::vec3), BufferUsage::DYNAMIC);
        m_VAO.addInstancedBuffer(m_instanceVBO, 3, 3); // Instance positions (x, y, z)

        m_instanceBufferCapacity = initialCapacity;
        m_buffersInitialized = true;
    }
    void addInstance(const glm::vec3& position);
    void updateInstanceBuffer();
    void resetInstances();
    void draw() const;

    [[nodiscard]] unsigned int m_instance_count() const;
};

#endif //INSTANCERENDERER_H
