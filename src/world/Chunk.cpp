#include "Chunk.h"

#include <cmath>

#include "World.h"
#include "vegetations/trees/Tree.h"

Chunk::Chunk(const int x, const int y, const int z) : Mesh(x, y, z) {
    constexpr size_t max_faces = 6 * 16 * 16 * 16;
    constexpr size_t avg_faces = max_faces / 4; // Assuming each block has a quarter of the maximum faces
    m_vertices.reserve(avg_faces * 4); // avg_faces * 4 vertices per face
    m_vertices_transparent.reserve(avg_faces * 4); // avg_faces * 4 vertices per face
    m_blockFaceData.reserve(avg_faces);
    m_blockFaceData_transparent.reserve(avg_faces);
    m_blockType.resize((SIZE + 2) * (SIZE + 2) * (SIZE + 2), BlockType::AIR); // +2 for boundary checks
}

void Chunk::generateVoxel(World &world) {
    std::array<std::array<int, SIZE + 2>, SIZE + 2> heightCache{};

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

            if (columnHeight < m_y - static_cast<int>(SIZE)) continue; // Early exit for aerial chunks, cast is mandatory

            // Pre-compute the max height for the current column
            constexpr int waterLevel = 63;
            const int maxHeightInChunk = std::max(columnHeight, waterLevel);
            const int endY = std::min(static_cast<int>(SIZE) + 2, std::max(0, maxHeightInChunk - m_y + 2));

            for (int localY = 0; localY < endY; localY++) {
                const int worldY = m_y + localY;

                if (worldY <= columnHeight) {
                    if (world.isCave(worldX, worldY, worldZ)) continue;
                    const BlockType blockType = Block::getBlockType(worldY, columnHeight);
                    m_blockType[index(localX, localY, localZ)] = blockType;
                } else if (worldY <= waterLevel) {
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

    if (!hasVisibleFaces()) {
        m_blockFaceData.shrink_to_fit();
        m_blockFaceData_transparent.shrink_to_fit();
        m_vertices.shrink_to_fit();
        m_vertices_transparent.shrink_to_fit();
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
    return (!m_vertices.empty() && !m_blockFaceData.empty()) ||
           (!m_vertices_transparent.empty() && !m_blockFaceData_transparent.empty());
}

bool Chunk::isBlockPresent(const int localX, const int localY, const int localZ) const {
    return m_blockType[index(localX+1, localY+1, localZ+1)] != BlockType::AIR;
}

void Chunk::addBlockFaces(const int localX, const int localY, const int localZ, const BlockType blockType) {
    if (blockType == BlockType::AIR) return;

    const auto localXf = static_cast<float>(localX);
    const auto localYf = static_cast<float>(localY);
    const auto localZf = static_cast<float>(localZ);
    const bool isTransparent = Block::isTransparent(blockType);

    if (shouldDrawFace(localX, localY + 1, localZ, blockType)) {
        if (isTransparent) {
            Block::addFaceVertices(Face::TOP, blockType, m_vertices_transparent, localXf, localYf, localZf);
            m_blockFaceData_transparent.emplace_back(Face::TOP, 4);
        } else {
            Block::addFaceVertices(Face::TOP, blockType, m_vertices, localXf, localYf, localZf);
            m_blockFaceData.emplace_back(Face::TOP, 4);
        }
    }
    if (shouldDrawFace(localX, localY - 1, localZ, blockType)) {
        if (isTransparent) {
            Block::addFaceVertices(Face::BOTTOM, blockType, m_vertices_transparent, localXf, localYf, localZf);
            m_blockFaceData_transparent.emplace_back(Face::BOTTOM, 4);
        } else {
            Block::addFaceVertices(Face::BOTTOM, blockType, m_vertices, localXf, localYf, localZf);
            m_blockFaceData.emplace_back(Face::BOTTOM, 4);
        }
    }
    if (shouldDrawFace(localX, localY, localZ + 1, blockType)) {
        if (isTransparent) {
            Block::addFaceVertices(Face::FRONT, blockType, m_vertices_transparent, localXf, localYf, localZf);
            m_blockFaceData_transparent.emplace_back(Face::FRONT, 4);
        } else {
            Block::addFaceVertices(Face::FRONT, blockType, m_vertices, localXf, localYf, localZf);
            m_blockFaceData.emplace_back(Face::FRONT, 4);
        }
    }
    if (shouldDrawFace(localX, localY, localZ - 1, blockType)) {
        if (isTransparent) {
            Block::addFaceVertices(Face::BACK, blockType, m_vertices_transparent, localXf, localYf, localZf);
            m_blockFaceData_transparent.emplace_back(Face::BACK, 4);
        } else {
            Block::addFaceVertices(Face::BACK, blockType, m_vertices, localXf, localYf, localZf);
            m_blockFaceData.emplace_back(Face::BACK, 4);
        }
    }
    if (shouldDrawFace(localX + 1, localY, localZ, blockType)) {
        if (isTransparent) {
            Block::addFaceVertices(Face::RIGHT, blockType, m_vertices_transparent, localXf, localYf, localZf);
            m_blockFaceData_transparent.emplace_back(Face::RIGHT, 4);
        } else {
            Block::addFaceVertices(Face::RIGHT, blockType, m_vertices, localXf, localYf, localZf);
            m_blockFaceData.emplace_back(Face::RIGHT, 4);
        }
    }
    if (shouldDrawFace(localX - 1, localY, localZ, blockType)) {
        if (isTransparent) {
            Block::addFaceVertices(Face::LEFT, blockType, m_vertices_transparent, localXf, localYf, localZf);
            m_blockFaceData_transparent.emplace_back(Face::LEFT, 4);
        } else {
            Block::addFaceVertices(Face::LEFT, blockType, m_vertices, localXf, localYf, localZf);
            m_blockFaceData.emplace_back(Face::LEFT, 4);
        }
    }
}

BlockType Chunk::getBlockType(const int localX, const int localY, const int localZ) const {
    return m_blockType[index(localX+1, localY+1, localZ+1)];
}
