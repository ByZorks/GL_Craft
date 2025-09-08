#include "Chunk.h"

#include <iostream>

#include "../TerrainGenerator.h"
#include "../WorldManager.h"

Chunk::Chunk(const int x, const int y, const int z) : Mesh(x, y, z, SIZE),
                                                      m_rng(TerrainGenerator::getSeed() + x + y + z) {
    constexpr int NUMBER_OF_FACES = 6;
    constexpr size_t max_faces = NUMBER_OF_FACES * SIZE * SIZE * SIZE;
    constexpr auto avg_vertices_opaque = static_cast<size_t>(max_faces * 0.02f);
    constexpr size_t avg_faces_water = SIZE * SIZE;

    m_opaqueData.vertices.reserve(avg_vertices_opaque);
    m_waterData.vertices.reserve(avg_faces_water);
    m_blocks.resize((SIZE + 2) * (SIZE + 2) * (SIZE + 2), Block::BlockType::AIR); // +2 for boundary checks
    m_lightLevels.resize((SIZE + 2) * (SIZE + 2) * (SIZE + 2), 0u);
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
            if (localX > 0 && localX < SIZE && localZ > 0 && localZ < SIZE &&
                // I think it can cause issues but I didn't find any in my testing
                columnHeight >= TerrainGenerator::getSeaLevel() && m_y <= columnHeight + 1 &&
                columnHeight >= m_y - static_cast<int>(SIZE) &&
                columnHeight < m_y + static_cast<int>(SIZE) &&
                !TerrainGenerator::isCave(position, worldX, columnHeight, worldZ, columnHeight, largeCavesNoises,
                                          tunnelCavesNoises)) {
                if (const float surfaceFeatureNoise =
                            (TerrainGenerator::getSurfaceFeaturesNoiseAt(worldX, worldZ) + 1.0f) * 0.5f;
                    surfaceFeatureNoise >= 0.69f) {
                    const Block::BlockType blockType = TerrainGenerator::getBlockType(columnHeight, columnHeight, biome);
                    m_surfaceFeatures.emplace(worldX, columnHeight, worldZ,
                                              SurfaceFeature::getSurfaceFeatureType(
                                                  surfaceFeatureNoise, blockType, biome));
                }
            }

            // Pre-compute the max height for the current column
            const int maxHeightInChunk = std::max(columnHeight, TerrainGenerator::getSeaLevel());
            const int endY = std::min(static_cast<int>(SIZE) + 2, std::max(0, maxHeightInChunk - m_y + 2));

            for (int localY = 0; localY < endY; localY++) {
                const int worldY = m_y + localY;

                // Terrain
                if (TerrainGenerator::isCave(position, worldX, worldY, worldZ, columnHeight, largeCavesNoises,
                                             tunnelCavesNoises)) continue;
                m_blocks[index(localX, localY, localZ)] = TerrainGenerator::getBlockType(worldY, columnHeight, biome);
                m_visibleBlocks++;

                // Surface features
                if (worldY != columnHeight + 1) continue;
                if (const auto it = m_surfaceFeatures.find(SurfaceFeature(worldX, columnHeight, worldZ));
                    it != m_surfaceFeatures.end()) {
                    switch (it->getType()) {
                        case SurfaceFeature::SurfaceFeatureType::TREE: {
                            SurfaceFeature::addTree(m_rng, {m_x, m_y, m_z}, localX, localY, localZ, biome, m_blocks,
                                                    m_pendingBlocksForNeighbors);
                            break;
                        }
                        case SurfaceFeature::SurfaceFeatureType::BUSH: {
                            SurfaceFeature::addBush(m_rng, {m_x, m_y, m_z}, localX, localY, localZ, biome, m_blocks,
                                                    m_pendingBlocksForNeighbors);
                            break;
                        }
                        case SurfaceFeature::SurfaceFeatureType::POND: {
                            SurfaceFeature::addPond(m_rng, {m_x, m_y, m_z}, localX, localY - 1, localZ, biome, m_blocks,
                                                    m_pendingBlocksForNeighbors);
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

    propagateLight();
    generateNewMesh(result);
}

void Chunk::propagateLight() {
    constexpr unsigned int MIN_LIGHT_LEVEL = 2u;
    constexpr unsigned int POS_MASK = 0x3F; // 6 bits

    std::queue<uint32_t> bfsQueue;

    // First pass: vertical light propagation from the top
    for (int localX = 0; localX < SIZE + 2; localX++) {
        const int adjustedX = localX - 1;

        for (int localZ = 0; localZ < SIZE + 2; localZ++) {
            const int adjustedZ = localZ - 1;
            unsigned int currentLightLevel = 15u;

            for (int localY = SIZE + 1; localY >= 0; localY--) {
                const int adjustedY = localY - 1;

                const Block::BlockType blockType = getBlockType(adjustedX, adjustedY, adjustedZ);

                if (Block::isTransparent(blockType)) {
                    setLightLevelAt(adjustedX, adjustedY, adjustedZ, currentLightLevel);
                    if (blockType != Block::BlockType::AIR) {
                        currentLightLevel = std::max(MIN_LIGHT_LEVEL, currentLightLevel - 2u);
                    }
                    if (currentLightLevel > MIN_LIGHT_LEVEL) {
                        // Use local values instead of adjusted to avoid negative values
                        bfsQueue.emplace(localX & POS_MASK | (localY & POS_MASK) << 6 | (localZ & POS_MASK) << 12);
                    }
                } else {
                    setLightLevelAt(adjustedX, adjustedY, adjustedZ, MIN_LIGHT_LEVEL);
                    currentLightLevel = MIN_LIGHT_LEVEL;
                }
            }
        }
    }

    // Second pass: horizontal light propagation (BFS algorithm)
    while (!bfsQueue.empty()) {
        const auto packed = bfsQueue.front();
        bfsQueue.pop();
        const int x = static_cast<int>(packed & POS_MASK) - 1;
        const int y = static_cast<int>(packed >> 6 & POS_MASK) - 1;
        const int z = static_cast<int>(packed >> 12 & POS_MASK) - 1;
        const uint8_t currentLightLevel = getLightLevelAt(x, y, z);
        if (currentLightLevel <= MIN_LIGHT_LEVEL) continue; // No more light to propagate

        for (auto [dx, dy, dz]: Block::s_faceOffset) {
            const int nx = x + dx;
            const int ny = y + dy;
            const int nz = z + dz;

            if (nx < -1 || ny < -1 || nz < -1 || nx >= SIZE + 1 || ny >= SIZE + 1 || nz >= SIZE + 1) continue;

            if (const Block::BlockType neighborBlockType = getBlockType(nx, ny, nz);
                Block::isOpaque(neighborBlockType)) continue;

            if (uint8_t neighborLightLevel = getLightLevelAt(nx, ny, nz);
                neighborLightLevel + 2 <= currentLightLevel) {
                neighborLightLevel = currentLightLevel - 1;
                setLightLevelAt(nx, ny, nz, neighborLightLevel);
                bfsQueue.emplace(nx + 1 & POS_MASK | (ny + 1 & POS_MASK) << 6 | (nz + 1 & POS_MASK) << 12);
            }
        }
    }
}

void Chunk::generateMesh() {
    for (int localX = 0; localX < SIZE; localX++) {
        for (int localY = 0; localY < SIZE; localY++) {
            for (int localZ = 0; localZ < SIZE; localZ++) {
                const Block::BlockType blockType = getBlockType(localX, localY, localZ);
                if (blockType == Block::BlockType::AIR) continue;

                // Check if block will have visible faces by checking its 6 neighbors
                uint8_t visibleFaces = 0;
                for (int dir = 0; dir < 6; dir++) {
                    if (const auto [dx, dy, dz] = Block::s_faceOffset[dir];
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
    for (int localX = 0; localX < SIZE; localX++) {
        for (int localY = 0; localY < SIZE; localY++) {
            for (int localZ = 0; localZ < SIZE; localZ++) {
                const Block::BlockType blockType = getBlockType(localX, localY, localZ);
                if (blockType == Block::BlockType::AIR) continue;

                // Check if block will have visible faces by checking its 6 neighbors
                uint8_t visibleFaces = 0;
                for (int dir = 0; dir < 6; dir++) {
                    if (const auto [dx, dy, dz] = Block::s_faceOffset[dir];
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

void Chunk::deleteBlock(const int localX, const int localY, const int localZ, const Block::BlockType type,
                        MeshingResult &result) {
    // Voxel
    m_visibleBlocks--;
    if (Block::isInstance(type)) {
        m_blocks[index(localX + 1, localY + 1, localZ + 1)] = Block::BlockType::AIR;
        m_surfaceFeatures.erase(SurfaceFeature(m_x + localX + 1, m_y + localY, m_z + localZ + 1));
        return;
    }

    m_blocks[index(localX + 1, localY + 1, localZ + 1)] = Block::BlockType::AIR;

    propagateLight();
    generateNewMesh(result);
}

void Chunk::addBlock(const int localX, const int localY, const int localZ, const Block::BlockType type,
                     MeshingResult &result) {
    // Voxel
    m_visibleBlocks++;
    if (Block::isInstance(type)) {
        m_blocks[index(localX + 1, localY + 1, localZ + 1)] = type;
        m_surfaceFeatures.emplace(m_x + localX + 1, m_y + localY, m_z + localZ + 1,
                                  SurfaceFeature::getSurfaceFeatureTypeFromBlockType(type));
        return;
    }
    m_blocks[index(localX + 1, localY + 1, localZ + 1)] = type;

    propagateLight();
    generateNewMesh(result);
}

int Chunk::index(const int x, const int y, const int z) const {
    constexpr int stride = static_cast<int>(SIZE) + 2;
    return x * stride * stride + y * stride + z;
}

Block::BlockType Chunk::getBlockType(const int localX, const int localY, const int localZ) const {
    return m_blocks[index(localX + 1, localY + 1, localZ + 1)];
}

Block::BlockType Chunk::getBlockTypeOrSurfaceFeature(const int localX, const int localY, const int localZ) const {
    Block::BlockType type;
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
    return m_drawIndexOpaque;
}

void Chunk::setOpaqueDrawIndex(const unsigned int m_draw_index) {
    m_drawIndexOpaque = m_draw_index;
}

unsigned int Chunk::getWaterDrawIndex() const {
    return m_drawIndexWater;
}

void Chunk::setWaterDrawIndex(const unsigned int m_water_draw_index) {
    m_drawIndexWater = m_water_draw_index;
}

unsigned int Chunk::getIndirectRendererSlotOpaque() const {
    return m_indirectRendererSlotOpaque;
}

void Chunk::setIndirectRendererSlotOpaque(const unsigned int m_gpu_opaque_slot) {
    m_indirectRendererSlotOpaque = m_gpu_opaque_slot;
}

unsigned int Chunk::getIndirectRendererSlotWater() const {
    return m_indirectRendererSlotWater;
}

void Chunk::setIndirectRendererSlotWater(const unsigned int m_gpu_water_slot) {
    m_indirectRendererSlotWater = m_gpu_water_slot;
}

void Chunk::addBlockFaces(const int localX, const int localY, const int localZ, const Block::BlockType blockType) {
    const auto localXf = static_cast<unsigned int>(localX);
    const auto localYf = static_cast<unsigned int>(localY);
    const auto localZf = static_cast<unsigned int>(localZ);
    const bool isWater = blockType == Block::BlockType::WATER;

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
                const bool isAdjacentBlockTransparent = Block::isTransparent(
                    getBlockType(localX + dx, localY + dy, localZ + dz));
                if (dy == 1 && blockType == Block::BlockType::WATER && !isTopBlockTransparent) {
                    adjacentFaces[index] = true; // AO is applied when top block is not transparent
                } else {
                    adjacentFaces[index] = !isAdjacentBlockTransparent;
                }
            }
        }
    }

    constexpr int NUMBER_OF_FACES = 6;
    for (int i = 0; i < NUMBER_OF_FACES; ++i) {
        const auto face = static_cast<Block::Face>(i);
        if (!shouldDrawFace(localX, localY, localZ, blockType, face)) continue;

        const auto [dx, dy, dz] = Block::s_faceOffset[i];
        const int nx = localX + dx;
        const int ny = localY + dy;
        const int nz = localZ + dz;
        const unsigned int lightLevel = getLightLevelAt(nx, ny, nz);

        if (isWater) {
            Block::addFaceVertex(face, blockType, m_waterData.vertices, adjacentFaces, localXf, localYf, localZf, lightLevel);
            m_waterData.hasFaces = true;
        } else {
            Block::addFaceVertex(face, blockType, m_opaqueData.vertices, adjacentFaces, localXf, localYf, localZf, lightLevel);
            m_opaqueData.hasFaces = true;
        }
    }
}

void Chunk::addBlockFaces(const int localX, const int localY, const int localZ, const Block::BlockType blockType,
                          MeshingResult &result) const {
    const auto localXf = static_cast<unsigned int>(localX);
    const auto localYf = static_cast<unsigned int>(localY);
    const auto localZf = static_cast<unsigned int>(localZ);
    const bool isWater = blockType == Block::BlockType::WATER;

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
                const bool isAdjacentBlockTransparent = Block::isTransparent(
                    getBlockType(localX + dx, localY + dy, localZ + dz));
                if (dy == 1 && blockType == Block::BlockType::WATER && !isTopBlockTransparent) {
                    adjacentFaces[index] = true; // AO is applied when top block is not transparent
                } else {
                    adjacentFaces[index] = !isAdjacentBlockTransparent;
                }
            }
        }
    }

    constexpr int NUMBER_OF_FACES = 6;
    for (int i = 0; i < NUMBER_OF_FACES; ++i) {
        const auto face = static_cast<Block::Face>(i);
        if (!shouldDrawFace(localX, localY, localZ, blockType, face)) continue;

        const auto [dx, dy, dz] = Block::s_faceOffset[i];
        const int nx = localX + dx;
        const int ny = localY + dy;
        const int nz = localZ + dz;
        const unsigned int lightLevel = getLightLevelAt(nx, ny, nz);

        if (isWater) {
            Block::addFaceVertex(face, blockType, result.waterVertices, adjacentFaces, localXf, localYf, localZf, lightLevel);
            result.hasWaterFaces = true;
        } else {
            Block::addFaceVertex(face, blockType, result.opaqueVertices, adjacentFaces, localXf, localYf, localZf, lightLevel);
            result.hasOpaqueFaces = true;
        }
    }
}

uint8_t Chunk::getLightLevelAt(const int localX, const int localY, const int localZ) const {
    return m_lightLevels[index(localX + 1, localY + 1, localZ + 1)];
}

void Chunk::setLightLevelAt(const int localX, const int localY, const int localZ, const uint8_t lightLevel) {
    m_lightLevels[index(localX + 1, localY + 1, localZ + 1)] = lightLevel;
}

bool Chunk::isBlockPresent(const int localX, const int localY, const int localZ) const {
    return m_blocks[index(localX + 1, localY + 1, localZ + 1)] != Block::BlockType::AIR;
}

bool Chunk::hasVisibleFaces() const {
    return !m_opaqueData.vertices.empty() || !m_waterData.vertices.empty();
}
