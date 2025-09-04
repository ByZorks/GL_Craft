#ifndef INSTANCERENDERER_H
#define INSTANCERENDERER_H

#include <array>

#include "../gl/StorageBuffer.h"
#include "../gl/VertexArray.h"
#include "../world/Block.h"
#include "../world/chunk/MeshManager.h"

class InstanceRenderer {
private:
    VertexArray m_VAO; // Only for binding
    StorageBuffer m_verticesSSBO;
    StorageBuffer m_instanceSSBO;
    std::vector<BlockVertex> m_vertices;

    std::vector<std::array<int, 3> > m_instancePositions;
    unsigned int m_instanceCount = 0;

public:
    template<typename MeshType>
    void init(MeshType &&mesh) {
        mesh.generateVoxel();
        mesh.generateMesh();

        m_vertices = mesh.getOpaqueVerticesCopy();

        if (!m_vertices.empty()) {
            m_VAO.init();
            m_verticesSSBO.init(m_vertices.data(), static_cast<unsigned int>(m_vertices.size() * sizeof(BlockVertex)),
                                0);
        } else {
            throw std::runtime_error("InstanceRenderer: Mesh has no vertices");
        }

        // Instance buffer
        constexpr size_t initialCapacity = 5000;
        m_instancePositions.reserve(initialCapacity);
        m_instanceSSBO.init(nullptr, initialCapacity * sizeof(std::array<int, 3>), 1);
    }

    void addInstance(const std::array<int, 3> &position);
    void updateInstanceBuffer();
    void resetInstances();
    void draw() const;

    [[nodiscard]] unsigned int getInstancesCount() const;
};

#endif //INSTANCERENDERER_H
