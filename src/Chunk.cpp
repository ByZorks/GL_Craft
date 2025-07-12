#include "Chunk.h"

#include "Block.h"
#include "Renderer.h"

unsigned int Chunk::m_size = 16; // Default chunk size

Chunk::Chunk(const int x, const int y, const int z) : m_xStart(x), m_yStart(y), m_zStart(z),
                                                      m_box(AABB(static_cast<float>(x), static_cast<float>(y),
                                                                 static_cast<float>(z),
                                                                 static_cast<float>(x) + static_cast<float>(m_size) - 1,
                                                                 static_cast<float>(y) + static_cast<float>(m_size) - 1,
                                                                 static_cast<float>(z) + static_cast<float>(m_size) -1
                                                                 )) {
}

Chunk::~Chunk() = default;

void Chunk::generate() {
    for (int localX = 0; localX < m_size; localX++) {
        for (int localY = 0; localY < m_size; localY++) {
            for (int localZ = 0; localZ < m_size; localZ++) {
                const auto worldX = static_cast<float>(m_xStart + localX);
                const auto worldY = static_cast<float>(m_yStart + localY);
                const auto worldZ = static_cast<float>(m_zStart + localZ);

                Block block(worldX, worldY, worldZ, 0.0f);
                const float *blockVertices = block.getVertices();
                m_vertices.insert(m_vertices.end(), blockVertices, blockVertices + 120);
            }
        }
    }
}

void Chunk::setupBuffers() {
    std::vector<unsigned int> chunkIndices;
    const unsigned int *blockIndices = Block::getIndices();

    // Calculate indices for each block in the chunk
    const unsigned int numBlocks = m_size * m_size * m_size;
    for (int i = 0; i < numBlocks; i++) {
        for (int j = 0; j < 36; j++) {
            chunkIndices.push_back(blockIndices[j] + i * 24); // 24 vertices per block
        }
    }

    m_VBO.init(m_vertices.data(), sizeof(float) * m_vertices.size());
    m_IBO.init(chunkIndices.data(), chunkIndices.size());

    VertexBufferLayout chunkLayout;
    chunkLayout.Push<float>(3); // x, y, z
    chunkLayout.Push<float>(2); // u, v
    m_VAO.AddBuffer(m_VBO, chunkLayout);
}

const VertexArray &Chunk::m_vao() const {
    return m_VAO;
}

const IndexBuffer &Chunk::m_ibo() const {
    return m_IBO;
}

unsigned int Chunk::m_size1() {
    return m_size;
}

const AABB & Chunk::m_box1() const {
    return m_box;
}
