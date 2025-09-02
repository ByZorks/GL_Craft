#include "Chunk.h"

#include <iostream>

#include "../TerrainGenerator.h"
#include "../WorldManager.h"

Chunk::Chunk(const int x, const int y, const int z) : Mesh(x, y, z, SIZE), m_rng(TerrainGenerator::getSeed() + x + y + z) {
    constexpr int NUMBER_OF_FACES = 6;
    constexpr size_t max_faces = NUMBER_OF_FACES * SIZE * SIZE * SIZE;
    constexpr size_t avg_vertices_opaque = max_faces * static_cast<size_t>(0.02f);
    constexpr size_t avg_faces_water = SIZE * SIZE;

    m_opaqueData.vertices.reserve(avg_vertices_opaque);
    m_waterData.vertices.reserve(avg_faces_water);
    m_blocks.resize((SIZE + 2) * (SIZE + 2) * (SIZE + 2), BlockType::AIR); // +2 for boundary checks
    m_pendingBlocksForNeighbors.reserve(SIZE);
    m_surfaceFeatures.reserve(SIZE * SIZE * 0.25f);
}

void Chunk::generateVoxel() {
    // Pre calculate noises values at 4x down sampling
    constexpr int step4 = 4;
    constexpr int gridSizeX4 = (SIZE + 2 + step4 - 1) / step4 + 1;
    constexpr int gridSizeY4 = gridSizeX4;
    constexpr int gridSizeZ4 = gridSizeX4;
    std::array<float, gridSizeX4 * gridSizeY4 * gridSizeZ4> tunnelCavesNoises{};
    for (int gx = 0; gx < gridSizeX4; ++gx) {
        const int wx = m_x + gx * step4;

        for (int gy = 0; gy < gridSizeY4; ++gy) {
            const int wy = m_y + gy * step4;

            for (int gz = 0; gz < gridSizeZ4; ++gz) {
                const int wz = m_z + gz * step4;

                const int index = gx + gridSizeX4 * (gy + gridSizeY4 * gz);
                tunnelCavesNoises[index] = TerrainGenerator::getTunnelCaveNoiseAt(wx, wy, wz);
            }
        }
    }

    // Pre calculate noises values at 12x down sampling
    constexpr int step8 = 8;
    constexpr int gridSizeX8 = (SIZE + 2 + step8 - 1) / step8 + 1;
    constexpr int gridSizeY8 = gridSizeX8;
    constexpr int gridSizeZ8 = gridSizeX8;
    std::array<float, gridSizeX8 * gridSizeY8 * gridSizeZ8> largeCavesNoises{};
    for (int gx = 0; gx < gridSizeX8; ++gx) {
        const int wx = m_x + gx * step8;

        for (int gy = 0; gy < gridSizeY8; ++gy) {
            const int wy = m_y + gy * step8;

            for (int gz = 0; gz < gridSizeZ8; ++gz) {
                const int wz = m_z + gz * step8;

                const int index = gx + gridSizeX8 * (gy + gridSizeY8 * gz);
                largeCavesNoises[index] = TerrainGenerator::getLargeCaveNoiseAt(wx, wy, wz);
            }
        }
    }

    for (int localX = 0; localX < SIZE + 2; localX++) {
        const int worldX = m_x + localX;

        for (int localZ = 0; localZ < SIZE + 2; localZ++) {
            const int worldZ = m_z + localZ;

            TerrainGenerator::NoiseValues noises;
            noises.computeHeightNoises(worldX, worldZ);
            const int columnHeight = TerrainGenerator::getHeight(noises);

            // Early exit for aerial chunks, cast is mandatory
            if (columnHeight < m_y - static_cast<int>(SIZE)) continue;

            noises.computeRemainingNoises(worldX, worldZ);
            const Biome biome = TerrainGenerator::getBiome(noises, worldX, worldZ);
            const auto position = ChunkPosition(m_x, m_y, m_z);

            // Surface features noise
            if (localX > 0 && localX < SIZE && localZ > 0 && localZ < SIZE && // I think it can cause issues but I didn't find any in my testing
                columnHeight >= TerrainGenerator::getSeaLevel() && m_y <= columnHeight + 1 &&
                columnHeight >= m_y - static_cast<int>(SIZE) &&
                columnHeight < m_y + static_cast<int>(SIZE) &&
                !TerrainGenerator::isCave(position, worldX, columnHeight, worldZ, columnHeight, largeCavesNoises, tunnelCavesNoises)) {
                if (const float surfaceFeatureNoise = (TerrainGenerator::getSurfaceFeaturesNoiseAt(worldX, worldZ) + 1.0f) * 0.5f;
                    surfaceFeatureNoise >= 0.69f) {
                    const BlockType blockType = TerrainGenerator::getBlockType(columnHeight, columnHeight, biome);
                    m_surfaceFeatures.emplace(worldX, columnHeight, worldZ, SurfaceFeature::getSurfaceFeatureType(surfaceFeatureNoise, blockType, biome));
                }
            }

            // Pre-compute the max height for the current column
            const int maxHeightInChunk = std::max(columnHeight, TerrainGenerator::getSeaLevel());
            const int endY = std::min(static_cast<int>(SIZE) + 2, std::max(0, maxHeightInChunk - m_y + 2));

            for (int localY = 0; localY < endY; localY++) {
                const int worldY = m_y + localY;

                // Terrain
                if (TerrainGenerator::isCave(position, worldX, worldY, worldZ, columnHeight, largeCavesNoises, tunnelCavesNoises)) continue;
                m_blocks[index(localX, localY, localZ)] = TerrainGenerator::getBlockType(worldY, columnHeight, biome);
                m_visibleBlocks++;

                // Surface features
                if (worldY != columnHeight + 1) continue;
                if (const auto it = m_surfaceFeatures.find(SurfaceFeature(worldX, columnHeight, worldZ));
                    it != m_surfaceFeatures.end()) {
                    switch (it->getType()) {
                        case SurfaceFeatureType::TREE: {
                            SurfaceFeature::addTree(m_rng, {m_x, m_y, m_z}, localX, localY, localZ, biome, m_blocks, m_pendingBlocksForNeighbors);
                            break;
                        }
                        case SurfaceFeatureType::BUSH: {
                            SurfaceFeature::addBush(m_rng, {m_x, m_y, m_z}, localX, localY, localZ, biome, m_blocks, m_pendingBlocksForNeighbors);
                            break;
                        }
                        case SurfaceFeatureType::POND: {
                            SurfaceFeature::addPond(m_rng, {m_x, m_y, m_z}, localX, localY - 1, localZ, biome, m_blocks, m_pendingBlocksForNeighbors);
                        }
                        default: {
                        }
                    }
                }
            }
        }
    }

    if (isEmpty()) {
        m_opaqueData.shrinkVertices();
        m_waterData.shrinkVertices();
    }

    m_state = State::VOXEL_GENERATED;
}

void Chunk::generatePendingBlocks(std::vector<PendingBlock> &blocks, MeshingResult &result) {
    for (const auto &[localX, localY, localZ, blockType]: blocks) {
        m_blocks[index(localX, localY, localZ)] = blockType;
    }
    blocks.clear();

    generateNewMesh(result);
}

void Chunk::generateMesh() {
    constexpr std::array FaceOffset = {
            std::make_tuple(0, 1, 0),  // Up
            std::make_tuple(0, -1, 0), // Down
            std::make_tuple(-1, 0, 0), // Left
            std::make_tuple(1, 0, 0),  // Right
            std::make_tuple(0, 0, 1),  // Front
            std::make_tuple(0, 0, -1)  // Back
    };

    for (int localX = 0; localX < SIZE; localX++) {
        for (int localY = 0; localY < SIZE; localY++) {
            for (int localZ = 0; localZ < SIZE; localZ++) {
                const BlockType blockType = getBlockType(localX, localY, localZ);
                if (blockType == BlockType::AIR) continue;

                // Check if block will have visible faces by checking its 6 neighbors
                uint8_t visibleFaces = 0;
                for (int dir = 0; dir < 6; dir++) {
                    if (const auto [dx, dy, dz] = FaceOffset[dir];
                        Block::isTransparent(getBlockType(localX + dx, localY + dy, localZ + dz))) {
                        visibleFaces |= 1 << dir;
                        break;
                    }
                }

                if (visibleFaces != 0) {
                    addBlockFaces(localX, localY, localZ, blockType);
                }
            }
        }
    }

    updateVertexCount();

    if (!hasOpaqueFaces()) m_opaqueData.shrinkVertices();
    if (!hasWaterFaces()) m_waterData.shrinkVertices();

    m_state = State::READY_TO_DRAW;
}

void Chunk::generateNewMesh(MeshingResult &result) const {
    constexpr std::array FaceOffset = {
        std::make_tuple(0, 1, 0),  // Up
        std::make_tuple(0, -1, 0), // Down
        std::make_tuple(-1, 0, 0), // Left
        std::make_tuple(1, 0, 0),  // Right
        std::make_tuple(0, 0, 1),  // Front
        std::make_tuple(0, 0, -1)  // Back
    };

    for (int localX = 0; localX < SIZE; localX++) {
        for (int localY = 0; localY < SIZE; localY++) {
            for (int localZ = 0; localZ < SIZE; localZ++) {
                const BlockType blockType = getBlockType(localX, localY, localZ);
                if (blockType == BlockType::AIR) continue;

                // Check if block will have visible faces by checking its 6 neighbors
                uint8_t visibleFaces = 0;
                for (int dir = 0; dir < 6; dir++) {
                    if (const auto [dx, dy, dz] = FaceOffset[dir];
                        Block::isTransparent(getBlockType(localX + dx, localY + dy, localZ + dz))) {
                        visibleFaces |= 1 << dir;
                        break;
                    }
                }

                if (visibleFaces != 0) {
                    addBlockFaces(localX, localY, localZ, blockType, result);
                }
            }
        }
    }
}

void Chunk::transferPendingBlocksToWorld(WorldManager &world) {
    if (m_pendingBlocksForNeighbors.empty()) return;
    world.addPendingBlocks(m_pendingBlocksForNeighbors);
    m_pendingBlocksForNeighbors.clear();
}

void Chunk::deleteBlock(const int localX, const int localY, const int localZ, const BlockType type, MeshingResult &result) {
    // Voxel
    m_visibleBlocks--;
    if (Block::isInstance(type)) {
        m_blocks[index(localX + 1, localY + 1, localZ + 1)] = BlockType::AIR;
        m_surfaceFeatures.erase(SurfaceFeature(m_x + localX + 1, m_y + localY, m_z + localZ + 1));
        return;
    }

    m_blocks[index(localX + 1, localY + 1, localZ + 1)] = BlockType::AIR;

    // Mesh data
    generateNewMesh(result);
}

void Chunk::addBlock(const int localX, const int localY, const int localZ, const BlockType type, MeshingResult &result) {
    // Voxel
    m_visibleBlocks++;
    if (Block::isInstance(type)) {
        m_blocks[index(localX + 1, localY + 1, localZ + 1)] = type;
        m_surfaceFeatures.emplace(m_x + localX + 1, m_y + localY, m_z + localZ + 1, SurfaceFeature::getSurfaceFeatureTypeFromBlockType(type));
        return;
    }
    m_blocks[index(localX + 1, localY + 1, localZ + 1)] = type;

    // Mesh data
    generateNewMesh(result);
}

int Chunk::index(const int x, const int y, const int z) const {
    constexpr int stride = static_cast<int>(SIZE) + 2;
    return x * stride * stride + y * stride + z;
}

BlockType Chunk::getBlockType(const int localX, const int localY, const int localZ) const {
    return m_blocks[index(localX + 1, localY + 1, localZ + 1)];
}

BlockType Chunk::getBlockTypeOrSurfaceFeature(const int localX, const int localY, const int localZ) const {
    BlockType type;
    const int worldX = m_x + localX + 1;
    const int worldY = m_y + localY;
    const int worldZ = m_z + localZ + 1;
    if (const auto it = m_surfaceFeatures.find(SurfaceFeature(worldX, worldY, worldZ));
        it != m_surfaceFeatures.end()) {
        if (it->isMultiBlockFeature()) {
            type = getBlockType(localX, localY, localZ);
        } else {
            type = SurfaceFeature::getBlockTypeOfSurfaceFeature(it->getType());
        }
    } else {
        type = getBlockType(localX, localY, localZ);
    }
    return type;
}

const std::unordered_set<SurfaceFeature> &Chunk::getSurfaceFeatures() const {
    return m_surfaceFeatures;
}

unsigned int Chunk::getOpaqueDrawIndex() const {
    return m_opaqueDrawIndex;
}

void Chunk::setOpaqueDrawIndex(const unsigned int m_draw_index) {
    m_opaqueDrawIndex = m_draw_index;
}

unsigned int Chunk::getWaterDrawIndex() const {
    return m_waterDrawIndex;
}

void Chunk::setWaterDrawIndex(const unsigned int m_water_draw_index) {
    m_waterDrawIndex = m_water_draw_index;
}

unsigned int Chunk::getGPUSlotOpaque() const {
    return m_gpuOpaqueSlot;
}

void Chunk::setGPUSlotOpaque(const unsigned int m_gpu_opaque_slot) {
    m_gpuOpaqueSlot = m_gpu_opaque_slot;
}

unsigned int Chunk::getGPUSlotWater() const {
    return m_gpuWaterSlot;
}

void Chunk::setGPUSlotWater(const unsigned int m_gpu_water_slot) {
    m_gpuWaterSlot = m_gpu_water_slot;
}

void Chunk::addBlockFaces(const int localX, const int localY, const int localZ, const BlockType blockType) {
    const auto localXf = static_cast<unsigned int>(localX);
    const auto localYf = static_cast<unsigned int>(localY);
    const auto localZf = static_cast<unsigned int>(localZ);
    const bool isWater = blockType == BlockType::WATER;

    std::array<bool, 26> adjacentFaces{};
    const bool isTopBlockTransparent = Block::isTransparent(getBlockType(localX, localY + 1, localZ));
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
                const bool isAdjacentBlockTransparent = Block::isTransparent(getBlockType(localX + dx, localY + dy, localZ + dz));
                if (dy == 1 && blockType == BlockType::WATER && !isTopBlockTransparent) {
                    adjacentFaces[index] = true; // AO is applied when top block is not transparent
                } else {
                    adjacentFaces[index] = !isAdjacentBlockTransparent;
                }
            }
        }
    }

    constexpr int NUMBER_OF_FACES = 6;
    for (int i = 0; i < NUMBER_OF_FACES; ++i) {
        const auto face = static_cast<Face>(i);
        if (!shouldDrawFace(localX, localY, localZ, blockType, face)) continue;

        if (isWater) {
            Block::addFaceVertices(face, blockType, m_waterData.vertices, adjacentFaces, localXf, localYf, localZf);
            if (face == Face::TOP) {
                Block::addFaceVertices(Face::TOP_INVERSED, blockType, m_waterData.vertices, adjacentFaces, localXf, localYf, localZf);
            }
            m_waterData.hasFaces = true;
        } else {
            Block::addFaceVertices(face, blockType, m_opaqueData.vertices, adjacentFaces, localXf, localYf, localZf);
            m_opaqueData.hasFaces = true;
        }
    }
}

void Chunk::addBlockFaces(const int localX, const int localY, const int localZ, const BlockType blockType, MeshingResult &result) const {
    if (blockType == BlockType::AIR || Block::isInstance(blockType)) return;

    const auto localXf = static_cast<unsigned int>(localX);
    const auto localYf = static_cast<unsigned int>(localY);
    const auto localZf = static_cast<unsigned int>(localZ);
    const bool isWater = blockType == BlockType::WATER;

    std::array<bool, 26> adjacentFaces{};
    const bool isTopBlockTransparent = Block::isTransparent(getBlockType(localX, localY + 1, localZ));
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
                const bool isAdjacentBlockTransparent = Block::isTransparent(getBlockType(localX + dx, localY + dy, localZ + dz));
                if (dy == 1 && blockType == BlockType::WATER && !isTopBlockTransparent) {
                    adjacentFaces[index] = true; // AO is applied when top block is not transparent
                } else {
                    adjacentFaces[index] = !isAdjacentBlockTransparent;
                }
            }
        }
    }

    constexpr int NUMBER_OF_FACES = 6;
    for (int i = 0; i < NUMBER_OF_FACES; ++i) {
        const auto face = static_cast<Face>(i);
        if (!shouldDrawFace(localX, localY, localZ, blockType, face)) continue;

        if (isWater) {
            Block::addFaceVertices(face, blockType, result.waterVertices, adjacentFaces, localXf, localYf, localZf);
            if (face == Face::TOP) {
                Block::addFaceVertices(Face::TOP_INVERSED, blockType, result.waterVertices, adjacentFaces, localXf, localYf, localZf);
            }
            result.hasWaterFaces = true;
        } else {
            Block::addFaceVertices(face, blockType, result.opaqueVertices, adjacentFaces, localXf, localYf, localZf);
            result.hasOpaqueFaces = true;
        }
    }
}

bool Chunk::isBlockPresent(const int localX, const int localY, const int localZ) const {
    return m_blocks[index(localX + 1, localY + 1, localZ + 1)] != BlockType::AIR;
}

bool Chunk::hasVisibleFaces() const {
    return !m_opaqueData.vertices.empty() || !m_waterData.vertices.empty();
}
