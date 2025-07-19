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
    m_vertices.reserve(6 * 4 * 8 * m_size * m_size * m_size); // 6 faces, 4 vertices per face, 8 components per vertex, 16x16x16 blocks
    m_blockFaceData.reserve(6 * m_size * m_size * m_size); // 6 faces per block, 16x16x16 blocks
}

Chunk::~Chunk() = default;

void Chunk::generateVoxelData(const FastNoiseLite& noiseGenerator) {
    for (int localX = 0; localX < m_size; localX++) {
        for (int localZ = 0; localZ < m_size; localZ++) {
            const auto worldX = static_cast<float>(m_xStart + localX);
            const auto worldZ = static_cast<float>(m_zStart + localZ);

            // Calculate height for this specific block column
            const float noiseValue = (noiseGenerator.GetNoise(worldX, worldZ) + 1.0f) / 2.0f; // Normalize to [0, 1]
            const int columnHeight = static_cast<int>(noiseValue * 100.0f); // Scale to world height [0, 100]

            for (int localY = 0; localY < m_size; localY++) {
                const int worldY = m_yStart + localY;
                if (worldY < columnHeight || worldY == 60) {
                    m_blockPresent[localX][localY][localZ] = true;
                }
            }
        }
    }
    m_status = Status::GENERATED;
}

void Chunk::generateMeshData(const World &world) {
    std::vector noiseCache(m_size, std::vector(m_size, -1.0f));

    for (int localY = 0; localY < m_size; localY++) {
        for (int localZ = 0; localZ < m_size; localZ++) {
            for (int localX = 0; localX < m_size; localX++) {
                if (!isBlockPresentInLocal(localX, localY, localZ)) {
                    continue;
                }

                const auto worldX = static_cast<float>(m_xStart + localX);
                const auto worldY = static_cast<float>(m_yStart + localY);
                const auto worldZ = static_cast<float>(m_zStart + localZ);
                float noiseValue = noiseCache[localX][localZ];
                if (noiseValue == -1.0f) {
                    noiseValue = (world.m_noise_generator().GetNoise(worldX, worldZ) + 1.0f) / 2.0f;
                    noiseCache[localX][localZ] = noiseValue;
                }
                const float columnHeight = noiseValue * 100.0f; // Scale to world height [0, 100]

                BlockType blockType;
                if (worldY > 80) blockType = BlockType::STONE;
                else if (worldY > 60) blockType = BlockType::GRASS;
                else if (worldY >= columnHeight && worldY == 60) blockType = BlockType::WATER;
                else if (worldY == 0) blockType = BlockType::BEDROCK;
                else blockType = BlockType::STONE;

                addBlockFaces(worldX, worldY, worldZ, blockType, world);
            }
        }
    }

    m_blockFaceData.shrink_to_fit();
    m_vertices.shrink_to_fit();

    m_status = Status::MESH_GENERATED;
}

void Chunk::setupBuffers() {
    std::vector<unsigned int> chunkIndices;
    chunkIndices.reserve(m_blockFaceData.size() * 6); // 6 indices per face (2 triangles)
    unsigned int vertexOffset = 0;

    for (const auto&[faceType, vertexCount] : m_blockFaceData) {
        unsigned int baseIdx = vertexOffset;

        if (faceType == Face::BACK || faceType == Face::LEFT || faceType == Face::TOP) {
            chunkIndices.insert(chunkIndices.end(), {
                baseIdx, baseIdx + 1, baseIdx + 2,
                baseIdx, baseIdx + 2, baseIdx + 3
            });
        } else {
            chunkIndices.insert(chunkIndices.end(), {
                baseIdx, baseIdx + 2, baseIdx + 1,
                baseIdx, baseIdx + 3, baseIdx + 2
            });
        }

        vertexOffset += vertexCount;
    }

    chunkIndices.shrink_to_fit();

    m_VBO.init(m_vertices.data(), sizeof(float) * m_vertices.size());
    m_IBO.init(chunkIndices.data(), chunkIndices.size());

    VertexBufferLayout chunkLayout;
    chunkLayout.Push<float>(3); // x, y, z
    chunkLayout.Push<float>(2); // u, v
    chunkLayout.Push<float>(3); // nx, ny, nz (normal vector)
    m_VAO.AddBuffer(m_VBO, chunkLayout);

    m_status = Status::BUFFERS_SETUP;
}

bool Chunk::isBlockPresentInWorld(const float worldX, const float worldY, const float worldZ, const World &world) {
    const int localX = static_cast<int>(worldX) - m_xStart;
    const int localY = static_cast<int>(worldY) - m_yStart;
    const int localZ = static_cast<int>(worldZ) - m_zStart;

    // Check if the block is within the chunk's local coordinates
    if (localX >= 0 && localX < m_size &&
        localY >= 0 && localY < m_size &&
        localZ >= 0 && localZ < m_size) {
        return isBlockPresentInLocal(localX, localY, localZ);
    }

    // If the block is outside the local chunk coordinates, check if it exists in another chunk
    return isBlockPresentInAnotherChunk(worldX, worldY, worldZ, world);
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

Status Chunk::m_status1() const {
    return m_status;
}

void Chunk::addBlockFaces(const float worldX, const float worldY, const float worldZ, const BlockType blockType, const World &world) {
    float columnIndex;
    switch (blockType) {
        case BlockType::BEDROCK: columnIndex = 0; break;
        case BlockType::DIRT: columnIndex = 3; break;
        case BlockType::GRASS: columnIndex = 6; break;
        case BlockType::STONE: columnIndex = 9; break;
        case BlockType::WATER: columnIndex = 12; break;
        default: throw std::invalid_argument("Invalid block type");
    }

    constexpr float TEXTURE_WIDTH = 1.0f / 15.0f;
    const float u_base = columnIndex * TEXTURE_WIDTH;

    constexpr Face faceOrder[6] = {Face::TOP, Face::BOTTOM, Face::FRONT, Face::BACK, Face::RIGHT, Face::LEFT};

    const bool currentBlockTransparent = isTransparent(blockType);

    const bool faces[6] = {
        // TOP
        shouldDrawFace(worldX, worldY + 1, worldZ, currentBlockTransparent, world),
        // BOTTOM
        shouldDrawFace(worldX, worldY - 1, worldZ, currentBlockTransparent, world),
        // FRONT
        shouldDrawFace(worldX, worldY, worldZ + 1, currentBlockTransparent, world),
        // BACK
        shouldDrawFace(worldX, worldY, worldZ - 1, currentBlockTransparent, world),
        // RIGHT
        shouldDrawFace(worldX + 1, worldY, worldZ, currentBlockTransparent, world),
        // LEFT
        shouldDrawFace(worldX - 1, worldY, worldZ, currentBlockTransparent, world)
    };

    for (int i = 0; i < 6; i++) {
        if (faces[i]) {
            Block::addFaceVertices(faceOrder[i], blockType, &m_vertices, worldX, worldY, worldZ, u_base);
            m_blockFaceData.push_back({faceOrder[i], 4});
        }
    }
}

bool Chunk::shouldDrawFace(const float nx, const float ny, const float nz, const bool currentTransparent, const World &world) {
    return !isBlockPresentInWorld(nx, ny, nz, world) ||
           (isBlockPresentInWorld(nx, ny, nz, world) && isTransparent(getBlockTypeAt(nx, ny, nz, world)) && !currentTransparent);
}

bool Chunk::isTransparent(const BlockType blockType) {
    return blockType == BlockType::WATER;
}

BlockType Chunk::getBlockTypeAt(const float x, const float y, const float z, const World &world) {
    if (!isBlockPresentInWorld(x, y, z, world)) return BlockType::WATER; // Air is transparent

    const float noiseValue = (world.m_noise_generator().GetNoise(x, z) + 1.0f) / 2.0f;
    const float columnHeight = noiseValue * 100.0f;

    if (y > 80) return BlockType::STONE;
    if (y > 60) return BlockType::GRASS;
    if (y >= columnHeight && y == 60) return BlockType::WATER;
    if (y == 0) return BlockType::BEDROCK;
    return BlockType::STONE;
}

bool Chunk::isBlockPresentInAnotherChunk(const float worldX, const float worldY, const float worldZ, const World &world) {
    const int chunkSize = static_cast<int>(m_size);
    const int chunkX = static_cast<int>(std::floor(static_cast<float>(worldX) / static_cast<float>(chunkSize))) * chunkSize;
    const int chunkY = static_cast<int>(std::floor(static_cast<float>(worldY) / static_cast<float>(chunkSize))) * chunkSize;
    const int chunkZ = static_cast<int>(std::floor(static_cast<float>(worldZ) / static_cast<float>(chunkSize))) * chunkSize;

    const std::tuple<int, int, int> chunkKey = std::make_tuple(chunkX, chunkY, chunkZ);
    const int localX = static_cast<int>(worldX) - chunkX;
    const int localY = static_cast<int>(worldY) - chunkY;
    const int localZ = static_cast<int>(worldZ) - chunkZ;
    if (m_adjacentChunks.contains(chunkKey)) {
        return m_adjacentChunks[chunkKey]->isBlockPresentInLocal(localX, localY, localZ);
    }

    if (const Chunk* chunk = world.getChunk(chunkX, chunkY, chunkZ)) {
        m_adjacentChunks[chunkKey] = chunk;
        return chunk->isBlockPresentInLocal(localX, localY, localZ);
    }
    return false;
}
