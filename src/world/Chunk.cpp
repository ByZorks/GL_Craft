#include "Chunk.h"

#include "World.h"

Chunk::Chunk(const int x, const int y, const int z) : Mesh(x, y, z, SIZE) {
    constexpr size_t max_faces = 6 * SIZE * SIZE * SIZE;
    constexpr size_t avg_faces = max_faces / 4; // Assuming each block has a quarter of the maximum faces
    constexpr size_t avg_faces_transparent = avg_faces * static_cast<size_t>(0.2f); // Most don't have many transparent faces
    m_vertices.reserve(avg_faces * 4); // 4 vertices per face
    m_vertices_transparent.reserve(avg_faces_transparent * 4);
    m_blockFaceData.reserve(avg_faces);
    m_blockFaceData_transparent.reserve(avg_faces_transparent);
    m_blockType.resize((SIZE + 2) * (SIZE + 2) * (SIZE + 2), BlockType::AIR); // +2 for boundary checks
}

void Chunk::generateVoxel(World &world) {
    constexpr int waterLevel = 63;
    std::array<std::array<int, SIZE + 2>, SIZE + 2> heightCache{};
    for (int localX = 0; localX < SIZE + 2; localX++) {
        const int worldX = m_x + localX;
        for (int localZ = 0; localZ < SIZE + 2; localZ++) {
            const int worldZ = m_z + localZ;

            // HeightMap
            const int columnHeight = world.getHeight(worldX, worldZ);
            heightCache[localX][localZ] = columnHeight;

            // Surface features
            if ((columnHeight < waterLevel && m_y < columnHeight) ||
                columnHeight < m_y ||
                columnHeight >= m_y + SIZE ||
                world.isCave(worldX, columnHeight, worldZ, columnHeight)) continue;
            float surfaceFeatureNoise = (world.m_surface_features_noise().GetNoise(static_cast<float>(worldX), static_cast<float>(worldZ)) + 1.0f) * 0.5f;
            if (surfaceFeatureNoise < 0.69f) continue;
            m_surfaceFeatures.emplace(worldX, columnHeight, worldZ, getSurfaceFeatureType(surfaceFeatureNoise));
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

                // Terrain
                if (world.isCave(worldX, worldY, worldZ, columnHeight)) continue;
                const BlockType blockType = Block::getBlockType(worldY, columnHeight);
                m_blockType[index(localX, localY, localZ)] = blockType;

                // Surface features
                if (worldY != columnHeight + 1) continue;
                const auto it = m_surfaceFeatures.find(SurfaceFeature(worldX, columnHeight, worldZ));
                if (it != m_surfaceFeatures.end()) {
                    switch (it->type) {
                        case SurfaceFeatureType::TREE: {
                            addTree(localX, localY, localZ);
                            break;
                        }
                        default: {}
                    }
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
        m_blockFaceData_water.shrink_to_fit();
        m_vertices.shrink_to_fit();
        m_vertices_transparent.shrink_to_fit();
        m_vertices_water.shrink_to_fit();
    }

    m_status = Status::MESH_GENERATED;
}

int Chunk::index(const int x, const int y, const int z) const {
    constexpr int stride = static_cast<int>(SIZE) + 2;
    return x * stride * stride + y * stride + z;
}

bool Chunk::hasVisibleFaces() const {
    return (!m_vertices.empty() && !m_blockFaceData.empty()) ||
           (!m_vertices_transparent.empty() && !m_blockFaceData_transparent.empty());
}

const std::unordered_set<SurfaceFeature> & Chunk::m_surface_features() const {
    return m_surfaceFeatures;
}

bool Chunk::isBlockPresent(const int localX, const int localY, const int localZ) const {
    return m_blockType[index(localX+1, localY+1, localZ+1)] != BlockType::AIR;
}

void Chunk::addBlockFaces(const int localX, const int localY, const int localZ, const BlockType blockType) {
    if (blockType == BlockType::AIR) return;

    const auto localXf = static_cast<float>(localX);
    const auto localYf = static_cast<float>(localY);
    const auto localZf = static_cast<float>(localZ);
    const bool isWater = blockType == BlockType::WATER;
    const bool isTransparent = Block::isTransparent(blockType);

    if (shouldDrawFace(localX, localY, localZ, blockType, Face::TOP)) {
        if (isWater) {
            Block::addFaceVertices(Face::TOP, blockType, m_vertices_water, localXf, localYf, localZf);
            m_blockFaceData_water.emplace_back(Face::TOP, 4);
            Block::addFaceVertices(Face::TOP_INVERSED, blockType, m_vertices_water, localXf, localYf, localZf);
            m_blockFaceData_water.emplace_back(Face::TOP_INVERSED, 4);
        } else if (isTransparent) {
            Block::addFaceVertices(Face::TOP, blockType, m_vertices_transparent, localXf, localYf, localZf);
            m_blockFaceData_transparent.emplace_back(Face::TOP, 4);
        } else {
            Block::addFaceVertices(Face::TOP, blockType, m_vertices, localXf, localYf, localZf);
            m_blockFaceData.emplace_back(Face::TOP, 4);
        }
    }
    if (shouldDrawFace(localX, localY, localZ, blockType, Face::BOTTOM)) {
        if (isWater) {
            Block::addFaceVertices(Face::BOTTOM, blockType, m_vertices_water, localXf, localYf, localZf);
            m_blockFaceData_water.emplace_back(Face::BOTTOM, 4);
        } else if (isTransparent) {
            Block::addFaceVertices(Face::BOTTOM, blockType, m_vertices_transparent, localXf, localYf, localZf);
            m_blockFaceData_transparent.emplace_back(Face::BOTTOM, 4);
        } else {
            Block::addFaceVertices(Face::BOTTOM, blockType, m_vertices, localXf, localYf, localZf);
            m_blockFaceData.emplace_back(Face::BOTTOM, 4);
        }
    }
    if (shouldDrawFace(localX, localY, localZ, blockType, Face::FRONT)) {
        if (isWater) {
            Block::addFaceVertices(Face::FRONT, blockType, m_vertices_water, localXf, localYf, localZf);
            m_blockFaceData_water.emplace_back(Face::FRONT, 4);
        } if (isTransparent) {
            Block::addFaceVertices(Face::FRONT, blockType, m_vertices_transparent, localXf, localYf, localZf);
            m_blockFaceData_transparent.emplace_back(Face::FRONT, 4);
        } else {
            Block::addFaceVertices(Face::FRONT, blockType, m_vertices, localXf, localYf, localZf);
            m_blockFaceData.emplace_back(Face::FRONT, 4);
        }
    }
    if (shouldDrawFace(localX, localY, localZ, blockType, Face::BACK)) {
        if (isWater) {
            Block::addFaceVertices(Face::BACK, blockType, m_vertices_water, localXf, localYf, localZf);
            m_blockFaceData_water.emplace_back(Face::BACK, 4);
        } else if (isTransparent) {
            Block::addFaceVertices(Face::BACK, blockType, m_vertices_transparent, localXf, localYf, localZf);
            m_blockFaceData_transparent.emplace_back(Face::BACK, 4);
        } else {
            Block::addFaceVertices(Face::BACK, blockType, m_vertices, localXf, localYf, localZf);
            m_blockFaceData.emplace_back(Face::BACK, 4);
        }
    }
    if (shouldDrawFace(localX, localY, localZ, blockType, Face::RIGHT)) {
        if (isWater) {
            Block::addFaceVertices(Face::RIGHT, blockType, m_vertices_water, localXf, localYf, localZf);
            m_blockFaceData_water.emplace_back(Face::RIGHT, 4);
        } else if (isTransparent) {
            Block::addFaceVertices(Face::RIGHT, blockType, m_vertices_transparent, localXf, localYf, localZf);
            m_blockFaceData_transparent.emplace_back(Face::RIGHT, 4);
        } else {
            Block::addFaceVertices(Face::RIGHT, blockType, m_vertices, localXf, localYf, localZf);
            m_blockFaceData.emplace_back(Face::RIGHT, 4);
        }
    }
    if (shouldDrawFace(localX, localY, localZ, blockType, Face::LEFT)) {
        if (isWater) {
            Block::addFaceVertices(Face::LEFT, blockType, m_vertices_water, localXf, localYf, localZf);
            m_blockFaceData_water.emplace_back(Face::LEFT, 4);
        } else if (isTransparent) {
            Block::addFaceVertices(Face::LEFT, blockType, m_vertices_transparent, localXf, localYf, localZf);
            m_blockFaceData_transparent.emplace_back(Face::LEFT, 4);
        } else {
            Block::addFaceVertices(Face::LEFT, blockType, m_vertices, localXf, localYf, localZf);
            m_blockFaceData.emplace_back(Face::LEFT, 4);
        }
    }
}

void Chunk::addTree(const int localX, const int localY, const int localZ) {
    // TODO: Allow for generation across multiple chunks
    if (localX < 3 || localX >= SIZE - 2 ||
        localY >= SIZE - 5 ||
        localZ < 3 || localZ >= SIZE - 2) {
        return;
    }

    // Trunk: 1x5x1 = 5 blocks (y=0 to y=4)
    for (int y = 0; y < 5; ++y) {
        if (localY + y >= SIZE) continue;
        m_blockType[index(localX, localY + y, localZ)] = BlockType::LOG;
    }

    // Leaves: 5x2x5 = 50 blocks (y=3 to y=4)
    for (int y = 3; y < 5; y++) {
        for (int x = -2; x <= 2; x++) {
            for (int z = -2; z <= 2; z++) {
                if (x == 0 && z == 0) continue;
                if (localX + x > SIZE ||
                    localY + y > SIZE ||
                    localZ + z > SIZE) continue;
                m_blockType[index(localX + x, localY + y, localZ + z)] = BlockType::LEAVES;
            }
        }
    }

    // Leaves: 3x2x3 = 18 blocks (y=5 to y=6)
    for (int y = 5; y < 7; y++) {
        for (int x = -1; x <= 1; x++) {
            for (int z = -1; z <= 1; z++) {
                if (std::abs(x) == 1 && std::abs(z) == 1) continue;
                if (localX + x > SIZE ||
                    localY + y > SIZE ||
                    localZ + z > SIZE) continue;
                m_blockType[index(localX + x, localY + y, localZ + z)] = BlockType::LEAVES;
            }
        }
    }
}

BlockType Chunk::getBlockType(const int localX, const int localY, const int localZ) const {
    return m_blockType[index(localX+1, localY+1, localZ+1)];
}
