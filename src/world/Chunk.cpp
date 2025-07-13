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
    bool blockPresent[m_size][m_size][m_size] = {false};

    for (int localX = 0; localX < m_size; localX++) {
        for (int localY = 0; localY < m_size; localY++) {
            for (int localZ = 0; localZ < m_size; localZ++) {
                blockPresent[localX][localY][localZ] = true;
            }
        }
    }

    for (int localX = 0; localX < m_size; localX++) {
        for (int localY = 0; localY < m_size; localY++) {
            for (int localZ = 0; localZ < m_size; localZ++) {
                if (!blockPresent[localX][localY][localZ]) {
                    continue; // Skip empty positions
                }

                const auto worldX = static_cast<float>(m_xStart + localX);
                const auto worldY = static_cast<float>(m_yStart + localY);
                const auto worldZ = static_cast<float>(m_zStart + localZ);

                Block block(worldX, worldY, worldZ, 0.0f);

                constexpr unsigned int verticesPerFace = 4;
                // Check all 6 directions and add faces if no adjacent block
                // TOP face (Y+1)
                if (localY + 1 >= m_size || !blockPresent[localX][localY + 1][localZ]) {
                    block.addFace(TOP);
                    m_blockFaceData.push_back({TOP, verticesPerFace});
                }

                // BOTTOM face (Y-1)
                if (localY - 1 < 0 || !blockPresent[localX][localY - 1][localZ]) {
                    block.addFace(BOTTOM);
                    m_blockFaceData.push_back({BOTTOM, verticesPerFace});
                }

                // FRONT face (Z+1)
                if (localZ + 1 >= m_size || !blockPresent[localX][localY][localZ + 1]) {
                    block.addFace(FRONT);
                    m_blockFaceData.push_back({FRONT, verticesPerFace});
                }

                // BACK face (Z-1)
                if (localZ - 1 < 0 || !blockPresent[localX][localY][localZ - 1]) {
                    block.addFace(BACK);
                    m_blockFaceData.push_back({BACK, verticesPerFace});
                }

                // RIGHT face (X+1)
                if (localX + 1 >= m_size || !blockPresent[localX + 1][localY][localZ]) {
                    block.addFace(RIGHT);
                    m_blockFaceData.push_back({RIGHT, verticesPerFace});
                }

                // LEFT face (X-1)
                if (localX - 1 < 0 || !blockPresent[localX - 1][localY][localZ]) {
                    block.addFace(LEFT);
                    m_blockFaceData.push_back({LEFT, verticesPerFace});
                }

                // Add block vertices to the chunk's vertex list
                const float* blockVertices = block.getVertices();
                const unsigned int vertexCount = block.getVertexCount() * 5; // 5 components per vertex
                m_vertices.insert(m_vertices.end(), blockVertices, blockVertices + vertexCount);
            }
        }
    }
}

void Chunk::setupBuffers() {
    std::vector<unsigned int> chunkIndices;
    unsigned int vertexOffset = 0;

    for (const auto&[faceType, vertexCount] : m_blockFaceData) {
        unsigned int baseIdx = vertexOffset;

        switch (faceType) {
            case FRONT:
            case RIGHT:
            case TOP:
                chunkIndices.push_back(baseIdx);
                chunkIndices.push_back(baseIdx + 1);
                chunkIndices.push_back(baseIdx + 2);

                chunkIndices.push_back(baseIdx);
                chunkIndices.push_back(baseIdx + 2);
                chunkIndices.push_back(baseIdx + 3);
                break;

            case BACK:
            case LEFT:
            case BOTTOM:
                chunkIndices.push_back(baseIdx);
                chunkIndices.push_back(baseIdx + 2);
                chunkIndices.push_back(baseIdx + 1);

                chunkIndices.push_back(baseIdx);
                chunkIndices.push_back(baseIdx + 3);
                chunkIndices.push_back(baseIdx + 2);
                break;
        }

        vertexOffset += vertexCount;
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
