#include "Chunk.h"

#include <cmath>
#include <iostream>

#include "Block.h"
#include "World.h"
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

void Chunk::generateVoxelData() {
    for (int localX = 0; localX < m_size; localX++) {
        for (int localY = 0; localY < m_size; localY++) {
            for (int localZ = 0; localZ < m_size; localZ++) {
                m_blockPresent[localX][localY][localZ] = true;
            }
        }
    }
}

void Chunk::generateMeshData(const World * world) {
    if (!world) throw std::runtime_error("World pointer is null in Chunk::generateMeshData");

    for (int localX = 0; localX < m_size; localX++) {
        for (int localY = 0; localY < m_size; localY++) {
            for (int localZ = 0; localZ < m_size; localZ++) {
                if (!isBlockPresentInLocal(localX, localY, localZ)) {
                    continue;
                }

                const auto worldX = static_cast<float>(m_xStart + localX);
                const auto worldY = static_cast<float>(m_yStart + localY);
                const auto worldZ = static_cast<float>(m_zStart + localZ);

                Block block(worldX, worldY, worldZ, 0.0f);

                constexpr unsigned int verticesPerFace = 4;
                // Check all 6 directions and add faces if no adjacent block
                // TOP face (Y+1)
                if (!isBlockPresentInWorld(worldX, worldY + 1, worldZ, world)) {
                    block.addFace(TOP);
                    m_blockFaceData.push_back({TOP, verticesPerFace});
                }

                // BOTTOM face (Y-1)
                if (!isBlockPresentInWorld(worldX, worldY - 1, worldZ, world)) {
                    block.addFace(BOTTOM);
                    m_blockFaceData.push_back({BOTTOM, verticesPerFace});
                }

                // FRONT face (Z+1)
                if (!isBlockPresentInWorld(worldX, worldY, worldZ + 1, world)) {
                    block.addFace(FRONT);
                    m_blockFaceData.push_back({FRONT, verticesPerFace});
                }

                // BACK face (Z-1)
                if (!isBlockPresentInWorld(worldX, worldY, worldZ - 1, world)) {
                    block.addFace(BACK);
                    m_blockFaceData.push_back({BACK, verticesPerFace});
                }

                // RIGHT face (X+1)
                if (!isBlockPresentInWorld(worldX + 1, worldY, worldZ, world)) {
                    block.addFace(RIGHT);
                    m_blockFaceData.push_back({RIGHT, verticesPerFace});
                }

                // LEFT face (X-1)
                if (!isBlockPresentInWorld(worldX - 1, worldY, worldZ, world)) {
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
            case BACK:
            case LEFT:
            case TOP:
                chunkIndices.push_back(baseIdx);
                chunkIndices.push_back(baseIdx + 1);
                chunkIndices.push_back(baseIdx + 2);

                chunkIndices.push_back(baseIdx);
                chunkIndices.push_back(baseIdx + 2);
                chunkIndices.push_back(baseIdx + 3);
                break;

            case FRONT:
            case RIGHT:
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

bool Chunk::isBlockPresentInWorld(const float worldX, const float worldY, const float worldZ, const World *world) {
    if (!world) throw std::runtime_error("World pointer is null in Chunk::isBlockPresent");

    const int blockX = static_cast<int>(std::floor(worldX));
    const int blockY = static_cast<int>(std::floor(worldY));
    const int blockZ = static_cast<int>(std::floor(worldZ));

    const int chunkSize = static_cast<int>(m_size);
    const int chunkX = static_cast<int>(std::floor(static_cast<float>(blockX) / static_cast<float>(chunkSize))) * chunkSize;
    const int chunkY = static_cast<int>(std::floor(static_cast<float>(blockY) / static_cast<float>(chunkSize))) * chunkSize;
    const int chunkZ = static_cast<int>(std::floor(static_cast<float>(blockZ) / static_cast<float>(chunkSize))) * chunkSize;

    if (const Chunk* chunk = world->getChunk(chunkX, chunkY, chunkZ)) {
        const int localX = blockX - chunkX;
        const int localY = blockY - chunkY;
        const int localZ = blockZ - chunkZ;
        return chunk->isBlockPresentInLocal(localX, localY, localZ);
    }
    return false;
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

bool Chunk::isBlockPresentInLocal(const int localX, const int localY, const int localZ) const {
    return (localX >= 0 && localX < m_size &&
            localY >= 0 && localY < m_size &&
            localZ >= 0 && localZ < m_size) &&
           m_blockPresent[localX][localY][localZ];
}
