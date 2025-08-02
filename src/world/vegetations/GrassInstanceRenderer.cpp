#include "GrassInstanceRenderer.h"

#include "../Block.h"
#include "../../render/Renderer.h"

struct BlockVertex;

GrassInstanceRenderer::GrassInstanceRenderer() = default;

GrassInstanceRenderer::~GrassInstanceRenderer() = default;

void GrassInstanceRenderer::init() {
    if (m_buffersInitialized) return;

    // Short grass vertices and indices
    std::vector<BlockVertex> vertices;
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::SHORT_GRASS, vertices, 0.0f, 0.0f, 0.0f);
    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::SHORT_GRASS, vertices, 0.0f, 0.0f, 0.0f);

    const std::vector<unsigned int> indices = {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7
    };

    m_VBO.init(vertices.data(), vertices.size() * sizeof(BlockVertex));
    m_IBO.init(indices.data(), indices.size());

    // Geometry
    VertexBufferLayout meshLayout;
    meshLayout.Push<unsigned char>(3); // x, y, z
    meshLayout.Push<unsigned char>(2, true); // u, v
    meshLayout.PushInt<unsigned char>(1); // face
    m_VAO.init();
    m_VAO.AddBuffer(m_VBO, meshLayout);

    // Instance buffer
    constexpr size_t initialCapacity = 10000;
    m_instancePositions.reserve(initialCapacity);
    m_instanceVBO.init(nullptr, initialCapacity * sizeof(glm::vec3), BufferUsage::DYNAMIC);
    m_VAO.addInstancedBuffer(m_instanceVBO, 3, 3); // Attribut d'instance pour les positions

    m_instanceBufferCapacity = initialCapacity;
    m_buffersInitialized = true;
}

void GrassInstanceRenderer::addInstance(const glm::vec3& position) {
    if (m_instanceCount < m_instanceBufferCapacity) {
        if (m_instanceCount >= m_instancePositions.size()) {
            m_instancePositions.push_back(position);
        } else {
            m_instancePositions[m_instanceCount] = position;
        }
        m_instanceCount++;
    }
}

void GrassInstanceRenderer::updateInstanceBuffer() {
    if (m_instanceCount == 0 || !m_buffersInitialized)
        return;

    // Resize the instance buffer if necessary
    if (m_instanceCount > m_instanceBufferCapacity) {
        m_instanceBufferCapacity = m_instanceCount * 2;

        m_instanceVBO.deleteBuffer();
        m_instanceVBO.init(m_instancePositions.data(), m_instanceBufferCapacity * sizeof(glm::vec3), BufferUsage::DYNAMIC);

        m_VAO.addInstancedBuffer(m_instanceVBO, 3, 3);
    } else {
        m_instanceVBO.updateData(m_instancePositions.data(), m_instanceCount * sizeof(glm::vec3));
    }
}

void GrassInstanceRenderer::resetInstances() {
    m_instanceCount = 0;
}

void GrassInstanceRenderer::draw() const {
    if (m_instanceCount == 0 || !m_buffersInitialized) return;

    Renderer::drawInstanced(m_VAO, m_IBO, m_instanceCount);
}

unsigned int GrassInstanceRenderer::m_instance_count() const {
    return m_instanceCount;
}
