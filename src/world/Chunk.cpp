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
    constexpr size_t max_faces = 6 * 16 * 16 * 16;
    constexpr size_t avg_faces = max_faces / 4; // Assuming each block has a quarter of the maximum faces
    m_vertices.reserve(avg_faces * 4); // avg_faces * 4 vertices per face
    m_blockFaceData.reserve(avg_faces);
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

    m_status = Status::VOXEL_GENERATED;
}

void Chunk::generateMeshData(const World &world) {
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

    for (const auto& [faceType, vertexCount] : m_blockFaceData) {
        constexpr unsigned int faceIndicesCCW[6] = {0, 2, 1, 0, 3, 2};
        constexpr unsigned int faceIndicesCW[6]  = {0, 1, 2, 0, 2, 3};
        const unsigned int* indices = faceType == Face::BACK || faceType == Face::LEFT || faceType == Face::TOP
            ? faceIndicesCW
            : faceIndicesCCW;
        for (int i = 0; i < 6; ++i) {
            chunkIndices.push_back(vertexOffset + indices[i]);
        }
        vertexOffset += vertexCount;
    }

    m_VBO.init(m_vertices.data(), m_vertices.size() * sizeof(BlockVertex));
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
    if (blockType == BlockType::AIR) return;

    const bool currentBlockTransparent = Block::isTransparent(blockType);

    if (shouldDrawFace(worldX, worldY + 1, worldZ, currentBlockTransparent, world)) {
        Block::addFaceVertices(Face::TOP, blockType, m_vertices, worldX, worldY, worldZ);
        m_blockFaceData.emplace_back(Face::TOP, 4);
    }
    if (shouldDrawFace(worldX, worldY - 1, worldZ, currentBlockTransparent, world)) {
        Block::addFaceVertices(Face::BOTTOM, blockType, m_vertices, worldX, worldY, worldZ);
        m_blockFaceData.emplace_back(Face::BOTTOM, 4);
    }
    if (shouldDrawFace(worldX, worldY, worldZ + 1, currentBlockTransparent, world)) {
        Block::addFaceVertices(Face::FRONT, blockType, m_vertices, worldX, worldY, worldZ);
        m_blockFaceData.emplace_back(Face::FRONT, 4);
    }
    if (shouldDrawFace(worldX, worldY, worldZ - 1, currentBlockTransparent, world)) {
        Block::addFaceVertices(Face::BACK, blockType, m_vertices, worldX, worldY, worldZ);
        m_blockFaceData.emplace_back(Face::BACK, 4);
    }
    if (shouldDrawFace(worldX + 1, worldY, worldZ, currentBlockTransparent, world)) {
        Block::addFaceVertices(Face::RIGHT, blockType, m_vertices, worldX, worldY, worldZ);
        m_blockFaceData.emplace_back(Face::RIGHT, 4);
    }
    if (shouldDrawFace(worldX - 1, worldY, worldZ, currentBlockTransparent, world)) {
        Block::addFaceVertices(Face::LEFT, blockType, m_vertices, worldX, worldY, worldZ);
        m_blockFaceData.emplace_back(Face::LEFT, 4);
    }
}

bool Chunk::shouldDrawFace(const float nx, const float ny, const float nz, const bool currentTransparent, const World &world) {
    if (!isBlockPresentInWorld(nx, ny, nz, world)) {
        return true; // Air block, always draw face
    }

    const BlockType neighborType = getBlockTypeAt(nx, ny, nz, world);

    // Don't draw faces between water blocks
    if (currentTransparent && neighborType == BlockType::WATER) {
        return false;
    }

    const bool neighborTransparent = Block::isTransparent(neighborType);

    // Draw face if neighbor is transparent and current block is solid
    return neighborTransparent && !currentTransparent;
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

    // Check cache first
    const std::tuple<int, int, int> chunkKey = std::make_tuple(chunkX, chunkY, chunkZ);
    if (const auto it = m_adjacentChunks.find(chunkKey); it != m_adjacentChunks.end()) {
        return it->second->getBlockTypeAtLocal(neighborLocalX, neighborLocalY, neighborLocalZ);
    }

    // Get chunk from world and cache it
    if (const Chunk* chunk = world.getChunk(chunkX, chunkY, chunkZ)) {
        m_adjacentChunks[chunkKey] = chunk;
        return chunk->getBlockTypeAtLocal(neighborLocalX, neighborLocalY, neighborLocalZ);
    }

    return BlockType::AIR;
}

BlockType Chunk::getBlockTypeAtLocal(const int localX, const int localY, const int localZ) const {
    if (localX >= 0 && localX < m_size &&
        localY >= 0 && localY < m_size &&
        localZ >= 0 && localZ < m_size) {
        return m_blockType[localX][localY][localZ];
    }
    return BlockType::AIR;
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

    if (const auto it = m_adjacentChunks.find(chunkKey); it != m_adjacentChunks.end()) {
        return it->second->isBlockPresentInLocal(localX, localY, localZ);
    }

    if (const Chunk* chunk = world.getChunk(chunkX, chunkY, chunkZ)) {
        m_adjacentChunks[chunkKey] = chunk;
        return chunk->isBlockPresentInLocal(localX, localY, localZ);
    }
    return false;
}
