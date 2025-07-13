#include "Chunk.h"

#include "Block.h"
#include "../render/Renderer.h"

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
                block.addFace(TOP);
                block.addFace(BOTTOM);
                block.addFace(FRONT);
                block.addFace(BACK);
                block.addFace(LEFT);
                block.addFace(RIGHT);
                const float *blockVertices = block.getVertices();
                m_vertices.insert(m_vertices.end(), blockVertices, blockVertices + 120);
            }
        }
    }
}

void Chunk::setupBuffers() {
    std::vector<unsigned int> chunkIndices;

    // Create a temporary block to get the base indices pattern
    Block tempBlock(0.0f, 0.0f, 0.0f, 0.0f);
    tempBlock.addFace(TOP);
    tempBlock.addFace(BOTTOM);
    tempBlock.addFace(FRONT);
    tempBlock.addFace(BACK);
    tempBlock.addFace(LEFT);
    tempBlock.addFace(RIGHT);

    const unsigned int* baseIndices = tempBlock.getIndices();
    const unsigned int indicesPerBlock = tempBlock.getIndexCount();

    // Calculate indices for each block in the chunk
    const unsigned int numBlocks = m_size * m_size * m_size;
    for (unsigned int i = 0; i < numBlocks; i++) {
        for (unsigned int j = 0; j < indicesPerBlock; j++) {
            constexpr unsigned int verticesPerBlock = 24;
            chunkIndices.push_back(baseIndices[j] + i * verticesPerBlock);
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

int Chunk::m_x_start() const {
    return m_xStart;
}

int Chunk::m_y_start() const {
    return m_yStart;
}

int Chunk::m_z_start() const {
    return m_zStart;
}
