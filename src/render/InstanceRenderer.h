#ifndef INSTANCERENDERER_H
#define INSTANCERENDERER_H

#include <array>

#include "../gl/StorageBuffer.h"
#include "../gl/VertexArray.h"
#include "../world/Block.h"

class InstanceRenderer {
private:
    VertexArray m_VAO;
    StorageBuffer m_SSBO;
    StorageBuffer m_instanceSSBO;
    std::vector<BlockVertex> m_vertices;

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
        meshCopy.generateMesh(false);

        m_vertices = meshCopy.getOpaqueVertices();

        if (!m_vertices.empty()) {
            m_VAO.init();
            m_SSBO.init(m_vertices.data(), static_cast<unsigned int>(m_vertices.size() * sizeof(BlockVertex)), 1);
        }

        // Instance buffer
        constexpr size_t initialCapacity = 10000;
        m_instancePositions.reserve(initialCapacity);
        m_instanceSSBO.init(nullptr, initialCapacity * sizeof(std::array<int, 3>), 2);

        m_instanceBufferCapacity = initialCapacity;
        m_buffersInitialized = true;
    }
    void addInstance(const std::array<int, 3> &position);
    void updateInstanceBuffer();
    void resetInstances();
    void draw() const;

    [[nodiscard]] unsigned int getInstancesCount() const;

private:

};

#endif //INSTANCERENDERER_H
