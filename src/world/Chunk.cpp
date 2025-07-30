#include "Chunk.h"

#include <cmath>

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

void Chunk::generateVoxel(World &world) {
    std::array<std::array<int, SIZE + 2>, SIZE + 2> heightCache;

    for (int localX = 0; localX < SIZE + 2; localX++) {
        const int worldX = m_x + localX;
        for (int localZ = 0; localZ < SIZE + 2; localZ++) {
            const int worldZ = m_z + localZ;
            heightCache[localX][localZ] = world.getHeight(worldX, worldZ);
        }
    }

    for (int localX = 0; localX < SIZE + 2; localX++) {
        const int worldX = m_x + localX;

        for (int localZ = 0; localZ < SIZE + 2; localZ++) {
            const int worldZ = m_z + localZ;
            const int columnHeight = heightCache[localX][localZ];

            if (columnHeight < m_y - SIZE) continue; // Early exit for aerial chunks

            // Pre-compute the max height for the current column
            const int endY = std::min(static_cast<int>(SIZE) + 2, std::max(0, columnHeight - m_y + 2));

            for (int localY = 0; localY < endY; localY++) {
                const int worldY = m_y + localY;

                if (world.isCave(worldX, worldY, worldZ)) continue;

                if (worldY <= columnHeight) {
                    const BlockType blockType = Block::getBlockType(worldY, columnHeight);
                    m_blockType[index(localX, localY, localZ)] = blockType;

                    if (blockType != BlockType::GRASS || worldY != columnHeight) continue;

                    const float vegetationNoise = (world.m_surface_vegetation_generator().GetNoise(static_cast<float>(worldX), static_cast<float>(worldZ)) + 1.0f) * 0.5f;
                    if (vegetationNoise < 0.875f) continue;

                    const auto tree = std::make_shared<Tree>(m_x + localX - 4, m_y + localY, m_z + localZ - 4);
                    tree->generateVoxel();
                    m_vegetations.emplace_back(tree);
                } else if (constexpr int waterLevel = 63; worldY < waterLevel && worldY <= columnHeight) {
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
