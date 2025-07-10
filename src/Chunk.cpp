#include "Chunk.h"

#include "Block.h"
#include "Renderer.h"

Chunk::Chunk() = default;

Chunk::~Chunk() = default;

void Chunk::generate() {
    for (int x = 0; x < m_size; x++) {
        for (int y = 0; y < m_size; y++) {
            for (int z = 0; z < m_size; z++) {
                Block block(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), 0.0f);

                const float* blockVertices = block.getVertices();
                m_vertices.insert(m_vertices.end(), blockVertices, blockVertices + 120); // 120 floats per block
            }
        }
    }

    setupBuffers();
}

void Chunk::setupBuffers() {
    std::vector<unsigned int> chunkIndices;
    const unsigned int* blockIndices = Block::getIndices();

    // Calculate indices for each block in the chunk
    const unsigned int numBlocks = m_size * m_size * m_size;
    for (int i = 0; i < numBlocks; i++) {
        for (int j = 0; j < 36; j++) {
            chunkIndices.push_back(blockIndices[j] + i * 24); // 24 vertices per block
        }
    }

    m_VBO.init(m_vertices.data(), sizeof(float) * m_vertices.size());
    m_IBO.init(chunkIndices.data(), chunkIndices.size());

    m_layout.Push<float>(3); // x, y, z
    m_layout.Push<float>(2); // u, v
    m_VAO.AddBuffer(m_VBO, m_layout);
}

const VertexArray & Chunk::m_vao() const {
    return m_VAO;
}

const IndexBuffer & Chunk::m_ibo() const {
    return m_IBO;
}
