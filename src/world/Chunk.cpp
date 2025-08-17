#include "Chunk.h"

#include "World.h"

Chunk::Chunk(const int x, const int y, const int z) : Mesh(x, y, z, SIZE) {
    constexpr int VERTEX_PER_FACE = 4;
    constexpr int NUMBER_OF_FACES = 6;
    constexpr size_t max_faces = NUMBER_OF_FACES * SIZE * SIZE * SIZE;
    constexpr size_t avg_faces = max_faces / VERTEX_PER_FACE; // Assuming each block has a quarter of the maximum faces
    constexpr size_t avg_faces_transparent = avg_faces * static_cast<size_t>(0.2f);
    constexpr size_t avg_faces_water = avg_faces * static_cast<size_t>(0.2f);

    m_opaqueData.vertices.reserve(avg_faces * VERTEX_PER_FACE);
    m_transparentData.vertices.reserve(avg_faces_transparent * VERTEX_PER_FACE);
    m_waterData.vertices.reserve(avg_faces_water * VERTEX_PER_FACE);
    m_opaqueData.faces.reserve(avg_faces);
    m_transparentData.faces.reserve(avg_faces_transparent);
    m_waterData.faces.reserve(avg_faces_water);
    m_blockType.resize((SIZE + 2) * (SIZE + 2) * (SIZE + 2), BlockType::AIR); // +2 for boundary checks
    m_pendingBlocksForNeighbors.reserve(SIZE);
    m_surfaceFeatures.reserve(SIZE * SIZE * 0.25f);
}

void Chunk::generateVoxel() {
    for (int localX = 0; localX < SIZE + 2; localX++) {
        const int worldX = m_x + localX;

        for (int localZ = 0; localZ < SIZE + 2; localZ++) {
            const int worldZ = m_z + localZ;
            const int columnHeight = World::getHeight(worldX, worldZ);

            // Early exit for aerial chunks, cast is mandatory
            if (columnHeight < m_y - static_cast<int>(SIZE)) continue;

            // Surface features noise
            constexpr int waterLevel = 63;
            if (columnHeight >= waterLevel && m_y <= columnHeight + 1 &&
                columnHeight >= m_y - static_cast<int>(SIZE) &&
                columnHeight < m_y + static_cast<int>(SIZE) &&
                !World::isCave(worldX, columnHeight, worldZ, columnHeight)) {
                const float surfaceFeatureNoise = (World::getSurfaceFeaturesNoise().GetNoise(
                                                       static_cast<float>(worldX),
                                                       static_cast<float>(worldZ)) + 1.0f) * 0.5f;
                if (surfaceFeatureNoise >= 0.69f) {
                    const BlockType blockType = Block::getBlockType(columnHeight, columnHeight);
                    m_surfaceFeatures.emplace(worldX, columnHeight, worldZ, getSurfaceFeatureType(surfaceFeatureNoise, blockType));
                }
            }

            // Pre-compute the max height for the current column
            const int maxHeightInChunk = std::max(columnHeight, waterLevel);
            const int endY = std::min(static_cast<int>(SIZE) + 2, std::max(0, maxHeightInChunk - m_y + 2));

            for (int localY = 0; localY < endY; localY++) {
                const int worldY = m_y + localY;

                // Terrain
                if (World::isCave(worldX, worldY, worldZ, columnHeight)) continue;
                const BlockType blockType = Block::getBlockType(worldY, columnHeight);
                m_blockType[index(localX, localY, localZ)] = blockType;

                // Surface features
                if (worldY != columnHeight + 1) continue;
                if (const auto it = m_surfaceFeatures.find(SurfaceFeature(worldX, columnHeight, worldZ));
                    it != m_surfaceFeatures.end()) {
                    switch (it->type) {
                        case SurfaceFeatureType::TREE: {
                            addTree(localX, localY, localZ);
                            break;
                        }
                        default: {
                        }
                    }
                }
            }
        }
    }
    m_state = State::VOXEL_GENERATED;
}

void Chunk::generatePendingBlocks(std::vector<PendingBlock> &blocks) {
    for (const auto &[localX, localY, localZ, blockType]: blocks) {
        m_blockType[index(localX, localY, localZ)] = blockType;
        addBlockFaces(localX - 1, localY - 1, localZ - 1, blockType);
    }
    blocks.clear();

    m_state = State::MESH_GENERATED;
}

void Chunk::generateMesh(const bool setFlag) {
    for (int localX = 0; localX < SIZE; localX++) {
        for (int localZ = 0; localZ < SIZE; localZ++) {
            for (int localY = 0; localY < SIZE; localY++) {
                if (!isBlockPresent(localX, localY, localZ)) continue;

                addBlockFaces(localX, localY, localZ, getBlockType(localX, localY, localZ));
            }
        }
    }

    if (!hasVisibleFaces()) {
        m_opaqueData.shrinkBuffers();
        m_transparentData.shrinkBuffers();
        m_waterData.shrinkBuffers();
    }

    if (setFlag) m_state = State::MESH_GENERATED;
}

void Chunk::transferPendingBlocksToWorld(World &world) {
    if (m_pendingBlocksForNeighbors.empty()) return;
    world.addPendingBlocks(m_pendingBlocksForNeighbors);
    m_pendingBlocksForNeighbors.clear();
}

void Chunk::deleteBlock(const int localX, const int localY, const int localZ) {
    // Voxel
    m_blockType[index(localX + 1, localY + 1, localZ + 1)] = BlockType::AIR;

    // Generate temp mesh data
    resetMesh();
    generateMesh(false);
}

int Chunk::index(const int x, const int y, const int z) const {
    constexpr int stride = static_cast<int>(SIZE) + 2;
    return x * stride * stride + y * stride + z;
}

bool Chunk::hasVisibleFaces() const {
    return (!m_opaqueData.vertices.empty() && !m_opaqueData.faces.empty()) ||
           (!m_transparentData.vertices.empty() && !m_transparentData.faces.empty()) ||
           (!m_waterData.vertices.empty() && !m_waterData.faces.empty());
}

const std::unordered_set<SurfaceFeature> &Chunk::getSurfaceFeatures() const {
    return m_surfaceFeatures;
}

bool Chunk::isBlockPresent(const int localX, const int localY, const int localZ) const {
    return m_blockType[index(localX + 1, localY + 1, localZ + 1)] != BlockType::AIR;
}

void Chunk::addBlockFaces(const int localX, const int localY, const int localZ, const BlockType blockType) {
    if (blockType == BlockType::AIR || blockType == BlockType::SURFACE_FEATURE_BASE) return;

    const auto localXf = static_cast<float>(localX);
    const auto localYf = static_cast<float>(localY);
    const auto localZf = static_cast<float>(localZ);
    const bool isWater = blockType == BlockType::WATER;
    const bool isTransparent = Block::isTransparent(blockType);

    std::array<bool, 26> adjacentFaces{};
    for (int dx = -1; dx <= 1; ++dx) {
        const int adjustedDX = dx + 1;

        for (int dy = -1; dy <= 1; ++dy) {
            const int adjustedDY = dy + 1;

            for (int dz = -1; dz <= 1; ++dz) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                constexpr int STRIDE = 3;
                constexpr int STRIDE_SQ = STRIDE * STRIDE;
                constexpr int MIDDLE_INDEX = 13;

                const int adjustedDZ = dz + 1;
                int index = adjustedDX * STRIDE_SQ + adjustedDY * STRIDE + adjustedDZ;
                index = index < MIDDLE_INDEX ? index : index - 1;
                adjacentFaces[index] = !Block::isTransparent(getBlockType(localX + dx, localY + dy, localZ + dz));
            }
        }
    }

    constexpr int NUMBER_OF_FACES = 6;
    for (int i = 0; i < NUMBER_OF_FACES; ++i) {
        const auto face = static_cast<Face>(i);
        if (!shouldDrawFace(localX, localY, localZ, blockType, face)) continue;

        if (isWater) {
            Block::addFaceVertices(face, blockType, m_waterData.vertices, adjacentFaces, localXf, localYf, localZf);
            m_waterData.faces.emplace_back(face);
            if (face == Face::TOP) {
                Block::addFaceVertices(Face::TOP_INVERSED, blockType, m_waterData.vertices, adjacentFaces, localXf, localYf, localZf);
                m_waterData.faces.emplace_back(Face::TOP_INVERSED);
            }
        } else if (isTransparent) {
            Block::addFaceVertices(face, blockType, m_transparentData.vertices, adjacentFaces, localXf, localYf, localZf);
            m_transparentData.faces.emplace_back(face);
        } else {
            Block::addFaceVertices(face, blockType, m_opaqueData.vertices, adjacentFaces, localXf, localYf, localZf);
            m_opaqueData.faces.emplace_back(face);
        }
    }
}

void Chunk::addTree(const int localX, const int localY, const int localZ) {
    // Trunk: 1x5x1 = 5 blocks (y=0 to y=4)
    for (int y = 0; y < 5; ++y) {
        addFeatureBlocks(localX, localY + y, localZ, BlockType::LOG);
    }

    // Leaves: 5x2x5 = 50 blocks (y=3 to y=4)
    for (int y = 3; y < 5; y++) {
        for (int x = -2; x <= 2; x++) {
            for (int z = -2; z <= 2; z++) {
                if (x == 0 && z == 0) continue; // Skip the trunk position
                addFeatureBlocks(localX + x, localY + y, localZ + z, BlockType::LEAVES);
            }
        }
    }

    // Leaves: 3x2x3 = 18 blocks (y=5 to y=6)
    for (int y = 5; y < 7; y++) {
        for (int x = -1; x <= 1; x++) {
            for (int z = -1; z <= 1; z++) {
                if (std::abs(x) == 1 && std::abs(z) == 1) continue; // Skip corners
                addFeatureBlocks(localX + x, localY + y, localZ + z, BlockType::LEAVES);
            }
        }
    }
}

void Chunk::addFeatureBlocks(const int localX, const int localY, const int localZ, BlockType blockType) {
    if (localX >= 1 && localX <= SIZE &&
        localY >= 1 && localY <= SIZE &&
        localZ >= 1 && localZ <= SIZE) {
        m_blockType[index(localX, localY, localZ)] = blockType;
    } else {
        const int worldX = m_x + localX - 1;
        const int worldY = m_y + localY - 1;
        const int worldZ = m_z + localZ - 1;

        const int chunkX = static_cast<int>(std::floor(static_cast<float>(worldX) / SIZE)) * static_cast<int>(SIZE);
        const int chunkY = static_cast<int>(std::floor(static_cast<float>(worldY) / SIZE)) * static_cast<int>(SIZE);
        const int chunkZ = static_cast<int>(std::floor(static_cast<float>(worldZ) / SIZE)) * static_cast<int>(SIZE);

        const int newLocalX = worldX - chunkX + 1;
        const int newLocalY = worldY - chunkY + 1;
        const int newLocalZ = worldZ - chunkZ + 1;

        m_pendingBlocksForNeighbors[{chunkX, chunkY, chunkZ}].emplace_back(newLocalX, newLocalY, newLocalZ, blockType);
    }
}

BlockType Chunk::getBlockType(const int localX, const int localY, const int localZ) const {
    return m_blockType[index(localX + 1, localY + 1, localZ + 1)];
}
