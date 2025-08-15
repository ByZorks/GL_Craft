#ifndef INSTANCERENDERER_H
#define INSTANCERENDERER_H

#include <array>
#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"
#include "../world/Block.h"

class InstanceRenderer {
private:
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    VertexBuffer m_instanceVBO;
    IndexBuffer m_IBO;

    std::vector<std::array<int, 3>> m_instancePositions;
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

        const auto& vertices = meshCopy.getOpaqueVertices();
        const auto& faces = meshCopy.getOpaqueBlockFaces();

        constexpr int NUMBER_OF_FACES = 6;
        if (!vertices.empty()) {
            std::vector<unsigned int> meshIndices_opaque;
            meshIndices_opaque.reserve(faces.size() * NUMBER_OF_FACES); // 6 indices per face
            unsigned int vertexOffsetOpaque = 0;

            for (const auto &faceType : faces) {
                constexpr int VERTEX_COUNT = 4;
                constexpr unsigned int faceIndicesCCW[6] = {0, 2, 1, 0, 3, 2};
                constexpr unsigned int faceIndicesCW[6] = {0, 1, 2, 0, 2, 3};
                const unsigned int *indices = faceType == Face::BACK || faceType == Face::LEFT || faceType == Face::TOP
                                                  ? faceIndicesCW : faceIndicesCCW;

                for (int i = 0; i < NUMBER_OF_FACES; ++i) {
                    meshIndices_opaque.push_back(vertexOffsetOpaque + indices[i]);
                }
                vertexOffsetOpaque += VERTEX_COUNT;
            }

            m_VBO.init(vertices.data(), vertices.size() * sizeof(BlockVertex));
            m_IBO.init(meshIndices_opaque.data(), meshIndices_opaque.size());

            VertexBufferLayout meshLayout;
            meshLayout.PushInt<unsigned char>(3); // x, y, z
            meshLayout.PushInt<unsigned char>(2, true); // u, v
            meshLayout.PushInt<unsigned char>(1); // face
            meshLayout.PushInt<unsigned char>(1); // AO

            m_VAO.init();
            m_VAO.addBuffer(m_VBO, meshLayout);
        }

        // Instance buffer
        constexpr size_t initialCapacity = 10000;
        m_instancePositions.reserve(initialCapacity);
        m_instanceVBO.init(nullptr, initialCapacity * sizeof(std::array<int, 3>), BufferUsage::DYNAMIC);
        m_VAO.addInstancedBuffer(m_instanceVBO, 4, 3); // Instance positions (x, y, z)

        m_instanceBufferCapacity = initialCapacity;
        m_buffersInitialized = true;
    }
    void addInstance(const std::array<int, 3> &position);
    void updateInstanceBuffer();
    void resetInstances();
    void draw() const;

    [[nodiscard]] unsigned int getInstancesCount() const;
    [[nodiscard]] std::vector<std::array<int, 3>> & getInstancesPositions();
};

#endif //INSTANCERENDERER_H
