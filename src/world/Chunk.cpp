#include "Chunk.h"

#include <cmath>
#include <iostream>

#include "World.h"
#include "surface_vegetations/Tree.h"

Chunk::Chunk(const int x, const int y, const int z) : Mesh(x, y, z) {
    constexpr size_t max_faces = 6 * 16 * 16 * 16;
    constexpr size_t avg_faces = max_faces / 4; // Assuming each block has a quarter of the maximum faces
    m_vertices.reserve(avg_faces * 4); // avg_faces * 4 vertices per face
    m_blockFaceDataOpaque.reserve(avg_faces);
    m_blockType.resize((SIZE + 2) * (SIZE + 2) * (SIZE + 2), BlockType::AIR); // +2 for boundary checks
    m_vegetations.reserve(8);
}

Chunk::~Chunk() {
    m_vegetations.clear();
}

void Chunk::generateVoxel(const FastNoiseLite& noiseGenerator, const FastNoiseLite& surfaceVegetationGenerator, const FastNoiseLite& caveGenerator) {
    for (int localX = 0; localX < SIZE + 2; localX++) { // +2 for boundary checks
        const auto worldX = static_cast<float>(m_x + localX);

        for (int localZ = 0; localZ < SIZE + 2; localZ++) {
            constexpr float maxHeight = 256.0f;
            constexpr int baseHeight = 60;
            const auto worldZ = static_cast<float>(m_z + localZ);

            const float normalizedNoise = (noiseGenerator.GetNoise(worldX, worldZ) + 1.0f) / 2.0f; // Normalize to [0, 1]
            const float terrainShape = std::pow(normalizedNoise, 4.6f); // Create more plains and sharper mountains
            const float columnHeight = baseHeight + terrainShape * maxHeight; // Scale to world height

            for (int localY = 0; localY < SIZE + 2; localY++) {
                const auto worldY = static_cast<float>(m_y + localY);

                const float normalized3DNoise = (caveGenerator.GetNoise(worldX, worldY, worldZ) + 1.0f) / 2.0f; // Normalize to [0, 1]
                constexpr float baseCaveThreshold = 0.82f;
                const float surfaceModifier = 1.0f - std::clamp((worldY - baseHeight) / (maxHeight * 0.7f), 0.0f, 1.0f);
                const float caveThreshold = baseCaveThreshold + surfaceModifier * 0.15f; // Increase threshold near surface

                if (worldY <= columnHeight) {
                    const BlockType blockType = Block::getBlockType(worldY, columnHeight, normalized3DNoise, caveThreshold);
                    m_blockType[index(localX, localY, localZ)] = blockType;

                    if (blockType != BlockType::GRASS) continue;

                    const float vegetationNoise = (surfaceVegetationGenerator.GetNoise(worldX, worldZ) + 1.0f) / 2.0f; // Normalize to [0, 1]
                    if (vegetationNoise < 0.9f) continue;

                    const auto tree = std::make_shared<Tree>(m_x + localX, m_y + localY + 1, m_z + localZ);
                    tree->generateVoxel();
                    m_vegetations.emplace_back(tree);
                    // m_blockType[index(localX, localY + 8, localZ)] = BlockType::LEAVES;
                } else if (constexpr int waterLevel = 63; worldY < waterLevel) {
                    m_blockType[index(localX, localY, localZ)] = BlockType::WATER;
                } else {
                    break;
                }
            }
        }
    }

    m_status = Status::VOXEL_GENERATED;
}

void Chunk::generateMesh() {
    for (int localX = 0; localX < SIZE; localX++) {
        for (int localZ = 0; localZ < SIZE; localZ++) {
            for (int localY = 0; localY < SIZE; localY++) {
                if (!isBlockPresent(localX, localY, localZ)) continue;

                addBlockFaces(localX, localY, localZ, m_blockType[index(localX+1, localY+1, localZ+1)]);
            }
        }
    }

    for (const auto& vegetation : m_vegetations) {
        vegetation->generateMesh();
    }

    if (!hasVisibleFaces()) {
        m_blockFaceDataOpaque.shrink_to_fit();
        m_vertices.shrink_to_fit();
    }

    m_status = Status::MESH_GENERATED;
}

int Chunk::index(const int x, const int y, const int z) {
    constexpr int stride = static_cast<int>(SIZE) + 2;
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
    return !m_vertices.empty() && !m_blockFaceDataOpaque.empty();
}

const std::vector<std::shared_ptr<Vegetation>> & Chunk::m_vegetations1() const {
    return m_vegetations;
}

bool Chunk::isBlockPresent(const int localX, const int localY, const int localZ) const {
    return m_blockType[index(localX+1, localY+1, localZ+1)] != BlockType::AIR;
}

void Chunk::addBlockFaces(const int localX, const int localY, const int localZ, const BlockType blockType) {
    if (blockType == BlockType::AIR) return;

    const auto localXf = static_cast<float>(localX);
    const auto localYf = static_cast<float>(localY);
    const auto localZf = static_cast<float>(localZ);

    if (shouldDrawFace(localX, localY + 1, localZ, blockType)) {
        Block::addFaceVertices(Face::TOP, blockType, m_vertices, localXf, localYf, localZf);
        m_blockFaceDataOpaque.emplace_back(Face::TOP, 4);
    }
    if (shouldDrawFace(localX, localY - 1, localZ, blockType)) {
        Block::addFaceVertices(Face::BOTTOM, blockType, m_vertices, localXf, localYf, localZf);
        m_blockFaceDataOpaque.emplace_back(Face::BOTTOM, 4);
    }
    if (shouldDrawFace(localX, localY, localZ + 1, blockType)) {
        Block::addFaceVertices(Face::FRONT, blockType, m_vertices, localXf, localYf, localZf);
        m_blockFaceDataOpaque.emplace_back(Face::FRONT, 4);
    }
    if (shouldDrawFace(localX, localY, localZ - 1, blockType)) {
        Block::addFaceVertices(Face::BACK, blockType, m_vertices, localXf, localYf, localZf);
        m_blockFaceDataOpaque.emplace_back(Face::BACK, 4);
    }
    if (shouldDrawFace(localX + 1, localY, localZ, blockType)) {
        Block::addFaceVertices(Face::RIGHT, blockType, m_vertices, localXf, localYf, localZf);
        m_blockFaceDataOpaque.emplace_back(Face::RIGHT, 4);
    }
    if (shouldDrawFace(localX - 1, localY, localZ, blockType)) {
        Block::addFaceVertices(Face::LEFT, blockType, m_vertices, localXf, localYf, localZf);
        m_blockFaceDataOpaque.emplace_back(Face::LEFT, 4);
    }
}

BlockType Chunk::getBlockType(const int localX, const int localY, const int localZ) const {
    return m_blockType[index(localX+1, localY+1, localZ+1)];
}
