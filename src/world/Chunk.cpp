#include "Chunk.h"

#include <cmath>

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
    constexpr int waterLevel = 62;
    constexpr int baseHeight = 60;
    constexpr float maxHeight = 200.0f; // Max height variation

    for (int localX = 0; localX < m_size; localX++) {
        const auto worldX = static_cast<float>(m_xStart + localX);

        for (int localZ = 0; localZ < m_size; localZ++) {
            const auto worldZ = static_cast<float>(m_zStart + localZ);

            const float normalizedNoise = (noiseGenerator.GetNoise(worldX, worldZ) + 1.0f) / 2.0f; // Normalize to [0, 1]
            const float terrainShape = std::pow(normalizedNoise, 4.5f); // Create more plains and sharper mountains
            const int columnHeight = baseHeight + static_cast<int>(terrainShape * maxHeight); // Scale to world height

            for (int localY = 0; localY < m_size; localY++) {
                const int worldY = m_yStart + localY;
                if (worldY <= columnHeight) {
                    m_blockPresent[localX][localY][localZ] = true;
                    m_blockType[localX][localY][localZ] = Block::getBlockType(worldY, columnHeight);
                } else if (worldY < waterLevel) {
                    m_blockPresent[localX][localY][localZ] = true;
                    m_blockType[localX][localY][localZ] = BlockType::WATER;
                }
            }
        }
    }
    m_status = Status::GENERATED;
}

void Chunk::generateMeshData(const World &world) {
    std::vector noiseCache(m_size, std::vector(m_size, -1.0f));

    for (int localX = 0; localX < m_size; localX++) {
        const auto worldX = static_cast<float>(m_xStart + localX);

        for (int localZ = 0; localZ < m_size; localZ++) {
            const auto worldZ = static_cast<float>(m_zStart + localZ);

            for (int localY = 0; localY < m_size; localY++) {
                if (!isBlockPresentInLocal(localX, localY, localZ)) continue;

                const auto worldY = static_cast<float>(m_yStart + localY);

                addBlockFaces(worldX, worldY, worldZ, m_blockType[localX][localY][localZ], world);
            }
        }
    }

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
    if (blockType == BlockType::UNKNOWN) return;
    const float columnIndex = Block::getTextureColumnIndex(blockType);
    constexpr float TEXTURE_WIDTH = 1.0f / 15.0f;
    const float u_base = columnIndex * TEXTURE_WIDTH;

    constexpr Face faceOrder[6] = {Face::TOP, Face::BOTTOM, Face::FRONT, Face::BACK, Face::RIGHT, Face::LEFT};

    const bool currentBlockTransparent = Block::isTransparent(blockType);

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
    const BlockType neighborType = getBlockTypeAt(nx, ny, nz, world);
    const bool neighborTransparent = Block::isTransparent(neighborType);

    if (!isBlockPresentInWorld(nx, ny, nz, world)) {
        return true; // Air block, always draw face
    }

    if (neighborTransparent && !currentTransparent) {
        return true; // Solid block next to a transparent one
    }

    if (currentTransparent && neighborType == BlockType::WATER) {
        return false; // Don't draw faces between water blocks
    }

    return false; // Face is hidden by another solid block
}

BlockType Chunk::getBlockTypeAt(const float worldX, const float worldY, const float worldZ, const World &world) {
    const int localX = static_cast<int>(worldX) - m_xStart;
    const int localY = static_cast<int>(worldY) - m_yStart;
    const int localZ = static_cast<int>(worldZ) - m_zStart;

    if (localX >= 0 && localX < m_size &&
        localY >= 0 && localY < m_size &&
        localZ >= 0 && localZ < m_size) {
        return m_blockType[localX][localY][localZ];
    }

    // If not local
    const int chunkSize = static_cast<int>(m_size);
    const int chunkX = static_cast<int>(std::floor(worldX / static_cast<float>(chunkSize))) * chunkSize;
    const int chunkY = static_cast<int>(std::floor(worldY / static_cast<float>(chunkSize))) * chunkSize;
    const int chunkZ = static_cast<int>(std::floor(worldZ / static_cast<float>(chunkSize))) * chunkSize;

    const int neighborLocalX = static_cast<int>(worldX) - chunkX;
    const int neighborLocalY = static_cast<int>(worldY) - chunkY;
    const int neighborLocalZ = static_cast<int>(worldZ) - chunkZ;

    const std::tuple<int, int, int> chunkKey = std::make_tuple(chunkX, chunkY, chunkZ);
    if (m_adjacentChunks.contains(chunkKey)) {
        return m_adjacentChunks.at(chunkKey)->getBlockTypeAtLocal(neighborLocalX, neighborLocalY, neighborLocalZ);
    }

    if (const Chunk* chunk = world.getChunk(chunkX, chunkY, chunkZ)) {
        m_adjacentChunks[chunkKey] = chunk;
        return chunk->getBlockTypeAtLocal(neighborLocalX, neighborLocalY, neighborLocalZ);
    }

    return BlockType::UNKNOWN;
}

BlockType Chunk::getBlockTypeAtLocal(const int localX, const int localY, const int localZ) const {
    if (localX >= 0 && localX < m_size &&
        localY >= 0 && localY < m_size &&
        localZ >= 0 && localZ < m_size) {
        return m_blockType[localX][localY][localZ];
    }
    return BlockType::UNKNOWN;
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
