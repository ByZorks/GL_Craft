#include "Chunk.h"

#include <cmath>
#include <iostream>

#include "Block.h"
#include "World.h"
#include "../render/Renderer.h"

unsigned int Chunk::m_size = 16; // Default chunk size

Chunk::Chunk(const int x, const int y, const int z) : m_x(x), m_y(y), m_z(z),
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
    m_blockType.resize((m_size + 2) * (m_size + 2) * (m_size + 2), BlockType::AIR); // +2 for boundary checks
}

Chunk::~Chunk() {
    m_vertices.clear();
    m_vertices.shrink_to_fit();
    m_blockFaceData.clear();
    m_blockFaceData.shrink_to_fit();
}

void Chunk::generateVoxelData(const FastNoiseLite& noiseGenerator) {
    for (int localX = 0; localX < m_size + 2; localX++) { // +2 for boundary checks
        const auto worldX = static_cast<float>(m_x + localX);

        for (int localZ = 0; localZ < m_size + 2; localZ++) {
            constexpr float maxHeight = 256.0f;
            constexpr int baseHeight = 60;
            const auto worldZ = static_cast<float>(m_z + localZ);

            const float normalizedNoise = (noiseGenerator.GetNoise(worldX, worldZ) + 1.0f) / 2.0f; // Normalize to [0, 1]
            const float terrainShape = std::pow(normalizedNoise, 4.6f); // Create more plains and sharper mountains
            const int columnHeight = baseHeight + static_cast<int>(terrainShape * maxHeight); // Scale to world height

            for (int localY = 0; localY < m_size + 2; localY++) {
                if (const int worldY = m_y + localY; worldY <= columnHeight) {
                    m_blockType[index(localX, localY, localZ)] = Block::getBlockType(worldY, columnHeight);
                } else if (constexpr int waterLevel = 62; worldY < waterLevel) {
                    m_blockType[index(localX, localY, localZ)] = BlockType::WATER;
                } else {
                    break;
                }
            }
        }
    }

    m_status = Status::VOXEL_GENERATED;
}

void Chunk::generateMeshData() {
    for (int localX = 0; localX < m_size; localX++) {
        for (int localZ = 0; localZ < m_size; localZ++) {
            for (int localY = 0; localY < m_size; localY++) {
                if (!isBlockPresent(localX, localY, localZ)) continue;

                addBlockFaces(localX, localY, localZ, m_blockType[index(localX+1, localY+1, localZ+1)]);
            }
        }
    }

    if (!hasVisibleFaces()) {
        m_blockFaceData.shrink_to_fit();
        m_vertices.shrink_to_fit();
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
    chunkLayout.Push<unsigned char>(3); // x, y, z
    chunkLayout.Push<unsigned char>(2, true); // u, v
    chunkLayout.PushInt<unsigned char>(1); // face
    m_VAO.AddBuffer(m_VBO, chunkLayout);

    m_status = Status::BUFFERS_SETUP;
}

int Chunk::index(const int x, const int y, const int z) {
    const int stride = static_cast<int>(m_size) + 2;
    return x * stride * stride + y * stride + z;
}

bool Chunk::hasBlocks() {
    if (std::any_of(m_blockType.begin(), m_blockType.end(), [](const BlockType type) { return type != BlockType::AIR; })) {
        m_blockType.clear();
        return true;
    }
    return false;
}

bool Chunk::hasVisibleFaces() const {
    return !m_vertices.empty() && !m_blockFaceData.empty();
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

int Chunk::m_x1() const {
    return m_x;
}

int Chunk::m_y1() const {
    return m_y;
}

int Chunk::m_z1() const {
    return m_z;
}

bool Chunk::isBlockPresent(const int localX, const int localY, const int localZ) const {
    return m_blockType[index(localX+1, localY+1, localZ+1)] != BlockType::AIR;
}

Status Chunk::m_status1() const {
    return m_status;
}

void Chunk::addBlockFaces(const int localX, const int localY, const int localZ, const BlockType blockType) {
    if (blockType == BlockType::AIR) return;

    const bool currentBlockTransparent = Block::isTransparent(blockType);

    const auto localXf = static_cast<float>(localX);
    const auto localYf = static_cast<float>(localY);
    const auto localZf = static_cast<float>(localZ);

    if (shouldDrawFace(localX, localY + 1, localZ, currentBlockTransparent)) {
        Block::addFaceVertices(Face::TOP, blockType, m_vertices, localXf, localYf, localZf);
        m_blockFaceData.emplace_back(Face::TOP, 4);
    }
    if (shouldDrawFace(localX, localY - 1, localZ, currentBlockTransparent)) {
        Block::addFaceVertices(Face::BOTTOM, blockType, m_vertices, localXf, localYf, localZf);
        m_blockFaceData.emplace_back(Face::BOTTOM, 4);
    }
    if (shouldDrawFace(localX, localY, localZ + 1, currentBlockTransparent)) {
        Block::addFaceVertices(Face::FRONT, blockType, m_vertices, localXf, localYf, localZf);
        m_blockFaceData.emplace_back(Face::FRONT, 4);
    }
    if (shouldDrawFace(localX, localY, localZ - 1, currentBlockTransparent)) {
        Block::addFaceVertices(Face::BACK, blockType, m_vertices, localXf, localYf, localZf);
        m_blockFaceData.emplace_back(Face::BACK, 4);
    }
    if (shouldDrawFace(localX + 1, localY, localZ, currentBlockTransparent)) {
        Block::addFaceVertices(Face::RIGHT, blockType, m_vertices, localXf, localYf, localZf);
        m_blockFaceData.emplace_back(Face::RIGHT, 4);
    }
    if (shouldDrawFace(localX - 1, localY, localZ, currentBlockTransparent)) {
        Block::addFaceVertices(Face::LEFT, blockType, m_vertices, localXf, localYf, localZf);
        m_blockFaceData.emplace_back(Face::LEFT, 4);
    }
}

bool Chunk::shouldDrawFace(const int localX, const int localY, const int localZ, const bool currentTransparent) const {
    if (!isBlockPresent(localX, localY, localZ)) {
        return true; // Air block, always draw face
    }

    const BlockType neighborType = getBlockType(localX, localY, localZ);

    // Don't draw faces between water blocks
    if (currentTransparent && neighborType == BlockType::WATER) {
        return false;
    }

    const bool neighborTransparent = Block::isTransparent(neighborType);

    // Draw face if neighbor is transparent and current block is solid
    return neighborTransparent && !currentTransparent;
}

BlockType Chunk::getBlockType(const int localX, const int localY, const int localZ) const {
    return m_blockType[index(localX+1, localY+1, localZ+1)];
}
