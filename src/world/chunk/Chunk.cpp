#include "Chunk.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include "LightConstants.h"
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
    m_lightLevels.resize((SIZE + 2) * (SIZE + 2) * (SIZE + 2), 1u);
    m_pendingBlocksForNeighbors.reserve(26);
    m_pendingLightsForNeighbors.reserve(26);
    m_surfaceFeatures.reserve(static_cast<unsigned long long>(SIZE * SIZE * 0.25f));
}

void Chunk::generateVoxel() {
    // 3D Noises
    constexpr int step4 = 4;
    constexpr int gridSizeX4 = (SIZE + 2 + step4 - 1) / step4 + 1;
    constexpr int gridSizeY4 = gridSizeX4;
    constexpr int gridSizeZ4 = gridSizeX4;
    std::array<float, gridSizeX4 * gridSizeY4 * gridSizeZ4> tunnelCavesNoises{};
    std::span<float> tunnelCavesNoisesSpan{tunnelCavesNoises};
    getDownsampledNoises(step4, tunnelCavesNoisesSpan, TerrainGenerator::getTunnelCaveNoiseAt);

    constexpr int step8 = 8;
    constexpr int gridSizeX8 = (SIZE + 2 + step8 - 1) / step8 + 1;
    constexpr int gridSizeY8 = gridSizeX8;
    constexpr int gridSizeZ8 = gridSizeX8;
    std::array<float, gridSizeX8 * gridSizeY8 * gridSizeZ8> largeCavesNoises{};
    std::span<float> largeCavesNoisesSpan{largeCavesNoises};
    getDownsampledNoises(step8, largeCavesNoisesSpan, TerrainGenerator::getLargeCaveNoiseAt);

    // Voxel
    for (int localX = 0; localX < SIZE + 2; ++localX) {
        const int worldX = m_x + localX;

        for (int localZ = 0; localZ < SIZE + 2; ++localZ) {
            const int worldZ = m_z + localZ;

            processColumn(worldX, worldZ, localX, localZ, tunnelCavesNoisesSpan, largeCavesNoisesSpan);
        }
    }

    if (isEmpty()) {
        m_opaqueData.shrinkVertices();
        m_waterData.shrinkVertices();
    }

    m_state = State::VOXEL_GENERATED;
}

void Chunk::generatePendingBlocks(const std::list<PendingBlock> &blocks, MeshingResult &outResult) {
    for (const auto &[localX, localY, localZ, blockType]: blocks) {
        m_blocks[index(localX, localY, localZ)] = blockType;
    }

    propagateLight();
    generateNewMesh(outResult);
}

void Chunk::propagateLight() {
    // Reset all light levels before a full recompute to avoid stale values
    std::ranges::fill(m_lightLevels, static_cast<uint16_t>(LightConstants::SUN_MIN));

    std::queue<uint32_t> sunlightQueue;
    std::queue<uint32_t> blocklightQueue;

    // First pass: vertical light propagation from the top
    for (int localX = 0; localX < SIZE + 2; localX++) {
        const int adjustedX = localX - 1;

        for (int localZ = 0; localZ < SIZE + 2; localZ++) {
            const int adjustedZ = localZ - 1;

            TerrainGenerator::NoiseValues noises;
            noises.computeHeightNoises(m_x + adjustedX, m_z + adjustedZ);
            const bool inDirectSunlight = !(TerrainGenerator::getHeight(noises) > m_y + SIZE ||
                                            TerrainGenerator::getSeaLevel() > m_y + SIZE);

            unsigned int currentLightLevel = inDirectSunlight ? 15u : LightConstants::SUN_MIN;

            for (int localY = SIZE + 1; localY >= 0; localY--) {
                const int adjustedY = localY - 1;

                const Block::BlockType blockType = getBlockType(adjustedX, adjustedY, adjustedZ);

                if (Block::isLightEmitter(blockType)) {
                    setBlockLightRGBAt(adjustedX, adjustedY, adjustedZ, Block::getLightColor(blockType));
                    blocklightQueue.emplace(packLightPos(adjustedX, adjustedY, adjustedZ));
                }

                if (!inDirectSunlight) continue;

                if (Block::isTransparent(blockType)) {
                    setSunLightLevelAt(adjustedX, adjustedY, adjustedZ, static_cast<uint8_t>(currentLightLevel));
                    if (blockType != Block::BlockType::AIR) {
                        // use signed int to avoid unsigned underflow when subtracting
                        int signedLevel = static_cast<int>(currentLightLevel) - 2;
                        signedLevel = std::max(static_cast<int>(LightConstants::SUN_MIN), signedLevel);
                        currentLightLevel = static_cast<unsigned int>(signedLevel);
                    }
                    if (currentLightLevel > LightConstants::SUN_MIN) {
                        sunlightQueue.emplace(packLightPos(adjustedX, adjustedY, adjustedZ));
                    }
                } else {
                    setSunLightLevelAt(adjustedX, adjustedY, adjustedZ, LightConstants::SUN_MIN);
                    currentLightLevel = LightConstants::SUN_MIN;
                }
            }
        }
    }

    propagateSunLight(sunlightQueue);
    propagateBlockLight(blocklightQueue);

    emitBorderLights();
}

void Chunk::generateMesh() {
    for (int localX = 0; localX < SIZE; localX++) {
        for (int localZ = 0; localZ < SIZE; localZ++) {
            for (int localY = 0; localY < SIZE; localY++) {
                const Block::BlockType blockType = getBlockType(localX, localY, localZ);
                if (blockType == Block::BlockType::AIR) continue;

                // Check if block will have visible faces by checking its 6 neighbors
                bool hasVisibleFaces = false;
                for (int dir = 0; dir < 6; dir++) {
                    if (const auto [dx, dy, dz] = Block::s_faceOffset[dir];
                        Block::isTransparent(getBlockType(localX + dx, localY + dy, localZ + dz))) {
                        hasVisibleFaces = true;
                        break;
                    }
                }

                if (hasVisibleFaces) {
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

void Chunk::generateNewMesh(MeshingResult &outResult) const {
    for (int localX = 0; localX < SIZE; localX++) {
        for (int localZ = 0; localZ < SIZE; localZ++) {
            for (int localY = 0; localY < SIZE; localY++) {
                const Block::BlockType blockType = getBlockType(localX, localY, localZ);
                if (blockType == Block::BlockType::AIR) continue;

                // Check if block will have visible faces by checking its 6 neighbors
                bool hasVisibleFaces = false;
                for (int dir = 0; dir < 6; dir++) {
                    if (const auto [dx, dy, dz] = Block::s_faceOffset[dir];
                        Block::isTransparent(getBlockType(localX + dx, localY + dy, localZ + dz))) {
                        hasVisibleFaces = true;
                        break;
                    }
                }

                if (hasVisibleFaces) {
                    addBlockFaces(localX, localY, localZ, blockType, outResult);
                }
            }
        }
    }
}

void Chunk::transferPendingBlocksToWorld(WorldManager &world) {
    if (m_pendingBlocksForNeighbors.empty()) return;
    world.addPendingBlocks(m_pendingBlocksForNeighbors);
}

void Chunk::transferPendingLightsToWorld(WorldManager &world) {
    if (m_pendingLightsForNeighbors.empty()) return;
    world.addPendingLights(m_pendingLightsForNeighbors);
}

void Chunk::generatePendingLights(const std::list<PendingLight> &lights, MeshingResult &outResult) {
    if (lights.empty()) return;

    std::queue<uint32_t> sunLightQueue;
    std::queue<uint32_t> blockLightQueue;
    std::vector<LightPos> changed;
    changed.reserve(lights.size() * 8);

    processInitialLightSources(lights, sunLightQueue, blockLightQueue, changed);
    propagateSunLightBFS(sunLightQueue, changed);
    propagateBlockLightBFS(blockLightQueue, changed);

    if (changed.empty()) return;

    emitChangedLightsToNeighbors(changed);
    outResult.needIndirectRendererUpdate = true;
    outResult.needInstanceUpdate = false;
    generateNewMesh(outResult);
}

void Chunk::deleteBlock(const int localX, const int localY, const int localZ, const Block::BlockType type,
                        MeshingResult &outResult) {
    // Voxel
    m_visibleBlocks--;
    if (Block::isInstance(type)) {
        m_blocks[index(localX + 1, localY + 1, localZ + 1)] = Block::BlockType::AIR;
        m_surfaceFeatures.erase(SurfaceFeature(m_x + localX + 1, m_y + localY, m_z + localZ + 1));
        return;
    }

    m_blocks[index(localX + 1, localY + 1, localZ + 1)] = Block::BlockType::AIR;

    propagateLight();
    generateNewMesh(outResult);
}

void Chunk::addBlock(const int localX, const int localY, const int localZ, const Block::BlockType type,
                     MeshingResult &outResult) {
    // Voxel
    m_visibleBlocks++;
    if (Block::isInstance(type)) {
        m_blocks[index(localX + 1, localY + 1, localZ + 1)] = type;
        m_surfaceFeatures.emplace(m_x + localX + 1, m_y + localY, m_z + localZ + 1,
                                  SurfaceFeature::getSurfaceFeatureTypeFromBlockType(type));
        return;
    }
    m_blocks[index(localX + 1, localY + 1, localZ + 1)] = type;

    if (Block::isLightEmitter(type)) {
        propagateBlockLightFrom(localX, localY, localZ);
    } else {
        propagateLight();
    }
    generateNewMesh(outResult);
}

void Chunk::emitBorderLights() {
    auto emitXFace = [&](const bool positive) {
        const int sample = positive ? static_cast<int>(SIZE) - 1 : 0;
        const int emitCoord = positive ? -1 : static_cast<int>(SIZE);
        const int offset = (positive ? 1 : -1) * static_cast<int>(SIZE);
        std::list<PendingLight> batch;

        for (int y = 0; y < static_cast<int>(SIZE); ++y) {
            for (int z = 0; z < static_cast<int>(SIZE); ++z) {
                const uint8_t sunLvl = getSunLightLevelAt(sample, y, z);
                const RGBLight blockLight = getBlockLightRGBLevelAt(sample, y, z);
                if (sunLvl <= LightConstants::SUN_MIN + 1u && !blockLight.shouldPropagate(LightConstants::SUN_MIN)) {
                    continue;
                }
                batch.emplace_back(emitCoord, y, z, blockLight, sunLvl);
            }
        }
        if (!batch.empty()) {
            const ChunkPosition key{m_x + offset, m_y, m_z};
            m_pendingLightsForNeighbors[key].splice(m_pendingLightsForNeighbors[key].end(), batch);
        }
    };

    auto emitYFace = [&](const bool positive) {
        const int sample = positive ? static_cast<int>(SIZE) - 1 : 0;
        const int emitCoord = positive ? -1 : static_cast<int>(SIZE);
        const int offset = (positive ? 1 : -1) * static_cast<int>(SIZE);
        std::list<PendingLight> batch;

        for (int x = 0; x < static_cast<int>(SIZE); ++x) {
            for (int z = 0; z < static_cast<int>(SIZE); ++z) {
                const uint8_t sunLvl = getSunLightLevelAt(x, sample, z);
                const RGBLight blockLight = getBlockLightRGBLevelAt(x, sample, z);
                if (sunLvl <= LightConstants::SUN_MIN + 1u && !blockLight.shouldPropagate(LightConstants::SUN_MIN)) {
                    continue;
                }
                batch.emplace_back(x, emitCoord, z, blockLight, sunLvl);
            }
        }
        if (!batch.empty()) {
            const ChunkPosition key{m_x, m_y + offset, m_z};
            m_pendingLightsForNeighbors[key].splice(m_pendingLightsForNeighbors[key].end(), batch);
        }
    };

    auto emitZFace = [&](const bool positive) {
        const int sample = positive ? static_cast<int>(SIZE) - 1 : 0;
        const int emitCoord = positive ? -1 : static_cast<int>(SIZE);
        const int offset = (positive ? 1 : -1) * static_cast<int>(SIZE);
        std::list<PendingLight> batch;

        for (int x = 0; x < static_cast<int>(SIZE); ++x) {
            for (int y = 0; y < static_cast<int>(SIZE); ++y) {
                const uint8_t sunLvl = getSunLightLevelAt(x, y, sample);
                const RGBLight blockLight = getBlockLightRGBLevelAt(x, y, sample);
                if (sunLvl <= LightConstants::SUN_MIN + 1u && !blockLight.shouldPropagate(LightConstants::SUN_MIN)) {
                    continue;
                }
                batch.emplace_back(x, y, emitCoord, blockLight, sunLvl);
            }
        }
        if (!batch.empty()) {
            const ChunkPosition key{m_x, m_y, m_z + offset};
            m_pendingLightsForNeighbors[key].splice(m_pendingLightsForNeighbors[key].end(), batch);
        }
    };

    emitXFace(false); // X-
    emitXFace(true);  // X+
    emitYFace(false); // Y-
    emitYFace(true);  // Y+
    emitZFace(false); // Z-
    emitZFace(true);  // Z+
}

int Chunk::index(const int x, const int y, const int z) const {
    constexpr int stride = static_cast<int>(SIZE) + 2;
    return x * stride * stride + z * stride + y;
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

template<typename NoiseFunction>
void Chunk::getDownsampledNoises(const int factor, const std::span<float> &outNoises, NoiseFunction noiseFunction) const {
    const int gridSizeX = (static_cast<int>(SIZE) + 2 + factor - 1) / factor + 1;
    const int gridSizeY = gridSizeX;
    const int gridSizeZ = gridSizeX;

    for (int gx = 0; gx < gridSizeX; ++gx) {
        const int wx = m_x + gx * factor;

        for (int gy = 0; gy < gridSizeY; ++gy) {
            const int wy = m_y + gy * factor;

            for (int gz = 0; gz < gridSizeZ; ++gz) {
                const int wz = m_z + gz * factor;

                const int index = gx + gridSizeX * (gy + gridSizeY * gz);
                outNoises[index] = noiseFunction(wx, wy, wz);
            }
        }
    }
}

void Chunk::processColumn(const int worldX, const int worldZ, const int localX, const int localZ,
                          const std::span<float> &tunnelCavesNoises, const std::span<float> &largeCavesNoises) {
    TerrainGenerator::NoiseValues noises;
    noises.computeHeightNoises(worldX, worldZ);

    const int columnHeight = TerrainGenerator::getHeight(noises);

    // Early exit for aerial chunks
    if (columnHeight < m_y - static_cast<int>(SIZE)) return;

    noises.computeRemainingNoises(worldX, worldZ);
    const Biome biome = TerrainGenerator::getBiome(noises, worldX, worldZ);
    const ChunkPosition position{m_x, m_y, m_z};

    generateSurfaceFeaturesPositions(position, worldX, worldZ, localX, localZ, columnHeight, biome, tunnelCavesNoises,
                                     largeCavesNoises);

    fillColumnBlocks(position, worldX, worldZ, localX, localZ, columnHeight, biome, tunnelCavesNoises, largeCavesNoises);
}

void Chunk::generateSurfaceFeaturesPositions(const ChunkPosition &position, int worldX, int worldZ, const int localX,
                                             const int localZ, int columnHeight, const Biome biome,
                                             const std::span<float> &tunnelCavesNoises, const std::span<float> &largeCavesNoises) {
    if (localX <= 0 || localX >= SIZE || localZ <= 0 || localZ >= SIZE) return;
    if (columnHeight < TerrainGenerator::getSeaLevel()) return;
    if (m_y > columnHeight + 1) return;
    if (columnHeight < m_y - SIZE || columnHeight >= m_y + SIZE) return;
    if (TerrainGenerator::isCave(position, worldX, columnHeight, worldZ, columnHeight, largeCavesNoises,
                                 tunnelCavesNoises)) return;

    const float noise = (TerrainGenerator::getSurfaceFeaturesNoiseAt(worldX, worldZ) + 1.0f) * 0.5f;
    if (noise < 0.69f) return;

    const Block::BlockType blockType = TerrainGenerator::getBlockType(columnHeight, columnHeight, biome);
    m_surfaceFeatures.emplace(worldX, columnHeight, worldZ, SurfaceFeature::getSurfaceFeatureType(noise, blockType, biome));
}

void Chunk::fillColumnBlocks(const ChunkPosition &position, const int worldX, const int worldZ, const int localX,
                             const int localZ, const int columnHeight, const Biome biome,
                             const std::span<float> &tunnelCavesNoises, const std::span<float> &largeCavesNoises) {
    const int maxHeight = std::max(columnHeight, TerrainGenerator::getSeaLevel());
    const int endY = std::min(static_cast<int>(SIZE) + 2, std::max(0, maxHeight - m_y + 2));

    for (int localY = 0; localY < endY; ++localY) {
        const int worldY = m_y + localY;

        if (TerrainGenerator::isCave(position, worldX, worldY, worldZ, columnHeight, largeCavesNoises, tunnelCavesNoises))
            continue;

        m_blocks[index(localX, localY, localZ)] = TerrainGenerator::getBlockType(worldY, columnHeight, biome);
        m_visibleBlocks++;

        if (worldY == columnHeight + 1) {
            addSurfaceFeatureBlocks(worldX, columnHeight, worldZ, localX, localY, localZ, biome);
        }
    }
}

void Chunk::addSurfaceFeatureBlocks(const int worldX, const int columnHeight, const int worldZ, const int localX,
                                    const int localY, const int localZ, const Biome biome) {
    const auto it = m_surfaceFeatures.find(SurfaceFeature(worldX, columnHeight, worldZ));
    if (it == m_surfaceFeatures.end()) return;

    switch (it->getType()) {
        case SurfaceFeature::SurfaceFeatureType::TREE:
            SurfaceFeature::addTree(m_rng, {m_x, m_y, m_z}, localX, localY, localZ,
                                    biome, m_blocks, m_pendingBlocksForNeighbors);
            break;
        case SurfaceFeature::SurfaceFeatureType::BUSH:
            SurfaceFeature::addBush(m_rng, {m_x, m_y, m_z}, localX, localY, localZ,
                                    biome, m_blocks, m_pendingBlocksForNeighbors);
            break;
        case SurfaceFeature::SurfaceFeatureType::POND:
            SurfaceFeature::addPond(m_rng, {m_x, m_y, m_z}, localX, localY - 1, localZ,
                                    biome, m_blocks, m_pendingBlocksForNeighbors);
            break;
        default: break;
    }
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
        const uint8_t sunLight = getSunLightLevelAt(nx, ny, nz);
        const RGBLight rgbLight = getBlockLightRGBLevelAt(nx, ny, nz);

        if (isWater) {
            Block::addFaceVertex(face, blockType, m_waterData.vertices, adjacentFaces, localXf, localYf, localZf,
                                 sunLight, rgbLight);
            m_waterData.hasFaces = true;
        } else {
            Block::addFaceVertex(face, blockType, m_opaqueData.vertices, adjacentFaces, localXf, localYf, localZf,
                                 sunLight, rgbLight);
            m_opaqueData.hasFaces = true;
        }
    }
}

void Chunk::addBlockFaces(const int localX, const int localY, const int localZ, const Block::BlockType blockType,
                          MeshingResult &outResult) const {
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
        const uint8_t sunLight = getSunLightLevelAt(nx, ny, nz);
        const RGBLight rgbLight = getBlockLightRGBLevelAt(nx, ny, nz);

        if (isWater) {
            Block::addFaceVertex(face, blockType, outResult.waterVertices, adjacentFaces, localXf, localYf, localZf,
                                 sunLight, rgbLight);
            outResult.hasWaterFaces = true;
        } else {
            Block::addFaceVertex(face, blockType, outResult.opaqueVertices, adjacentFaces, localXf, localYf, localZf,
                                 sunLight, rgbLight);
            outResult.hasOpaqueFaces = true;
        }
    }
}

void Chunk::processInitialLightSources(const std::list<PendingLight> &lights, std::queue<uint32_t> &sunLightQueue,
                                       std::queue<uint32_t> &blockLightQueue, std::vector<LightPos> &changed) {
    for (const auto &[localX, localY, localZ, blockLight, sunlight] : lights) {
        if (!isValidLightPosition(localX, localY, localZ)) continue;
        if (Block::isOpaque(getBlockType(localX, localY, localZ))) continue;

        const bool sunChanged = updateSunLight(localX, localY, localZ, sunlight, sunLightQueue);
        const bool rgbChanged = updateBlockLight(localX, localY, localZ, blockLight, blockLightQueue);

        if (sunChanged || rgbChanged) {
            changed.emplace_back(localX, localY, localZ);
        }
    }
}

bool Chunk::isValidLightPosition(const int x, const int y, const int z) {
    return x >= -1 && y >= -1 && z >= -1 &&
           x <= static_cast<int>(SIZE) &&
           y <= static_cast<int>(SIZE) &&
           z <= static_cast<int>(SIZE);
}

bool Chunk::updateSunLight(const int x, const int y, const int z, const uint8_t sunlight, std::queue<uint32_t> &queue) {
    if (sunlight <= getSunLightLevelAt(x, y, z)) return false;

    setSunLightLevelAt(x, y, z, sunlight);
    if (sunlight > LightConstants::SUN_MIN) {
        queue.emplace(packLightPos(x, y, z));
    }
    return true;
}

bool Chunk::updateBlockLight(const int x, const int y, const int z, const RGBLight &blockLight, std::queue<uint32_t> &queue) {
    const RGBLight currentRGB = getBlockLightRGBLevelAt(x, y, z);
    const RGBLight newRGB = {
        std::max(blockLight.r, currentRGB.r),
        std::max(blockLight.g, currentRGB.g),
        std::max(blockLight.b, currentRGB.b)
    };

    if (newRGB.r == currentRGB.r && newRGB.g == currentRGB.g && newRGB.b == currentRGB.b) {
        return false;
    }

    setBlockLightRGBAt(x, y, z, newRGB);
    if (newRGB.shouldPropagate(LightConstants::SUN_MIN)) {
        queue.emplace(packLightPos(x, y, z));
    }
    return true;
}

void Chunk::propagateSunLightBFS(std::queue<uint32_t> &sunLightQueue, std::vector<LightPos> &changed) {
    while (!sunLightQueue.empty()) {
        const auto [x, y, z] = unpackLightPos(sunLightQueue.front());
        sunLightQueue.pop();

        const uint8_t currentLightLevel = getSunLightLevelAt(x, y, z);
        if (currentLightLevel <= LightConstants::SUN_MIN) continue;

        propagateSunLightToNeighbors(x, y, z, currentLightLevel, sunLightQueue, changed);
    }
}

void Chunk::propagateSunLightToNeighbors(const int x, const int y, const int z, const uint8_t currentLevel,
                                         std::queue<uint32_t> &queue, std::vector<LightPos> &changed) {
    for (auto [dx, dy, dz] : Block::s_faceOffset) {
        const int nx = x + dx;
        const int ny = y + dy;
        const int nz = z + dz;

        if (!isValidLightPosition(nx, ny, nz)) continue;
        if (Block::isOpaque(getBlockType(nx, ny, nz))) continue;

        const uint8_t neighborLevel = getSunLightLevelAt(nx, ny, nz);
        const auto newLevel = static_cast<uint8_t>(currentLevel - 1u);

        if (neighborLevel + 2u <= currentLevel && newLevel > neighborLevel) {
            setSunLightLevelAt(nx, ny, nz, newLevel);
            changed.emplace_back(nx, ny, nz);
            queue.emplace(packLightPos(nx, ny, nz));
        }
    }
}

void Chunk::propagateBlockLightBFS(std::queue<uint32_t> &blockLightQueue, std::vector<LightPos> &changed) {
    while (!blockLightQueue.empty()) {
        const auto [x, y, z] = unpackLightPos(blockLightQueue.front());
        blockLightQueue.pop();

        const RGBLight currentRGB = getBlockLightRGBLevelAt(x, y, z);
        if (!currentRGB.shouldPropagate(LightConstants::SUN_MIN)) continue;

        propagateBlockLightToNeighbors(x, y, z, currentRGB, blockLightQueue, changed);
    }
}

void Chunk::propagateBlockLightToNeighbors(const int x, const int y, const int z, const RGBLight &currentRGB,
                                           std::queue<uint32_t> &queue, std::vector<LightPos> &changed) {
    for (auto [dx, dy, dz] : Block::s_faceOffset) {
        const int nx = x + dx;
        const int ny = y + dy;
        const int nz = z + dz;

        if (!isValidLightPosition(nx, ny, nz)) continue;
        if (Block::isOpaque(getBlockType(nx, ny, nz))) continue;

        if (tryUpdateBlockLightAt(nx, ny, nz, currentRGB)) {
            queue.emplace(packLightPos(nx, ny, nz));
            changed.emplace_back(nx, ny, nz);
        }
    }
}

bool Chunk::tryUpdateBlockLightAt(const int x, const int y, const int z, const RGBLight &sourceRGB) {
    const RGBLight neighborLight = getBlockLightRGBLevelAt(x, y, z);
    const RGBLight attenuatedLight = {
        static_cast<uint8_t>(std::max(0, static_cast<int>(sourceRGB.r) - 1)),
        static_cast<uint8_t>(std::max(0, static_cast<int>(sourceRGB.g) - 1)),
        static_cast<uint8_t>(std::max(0, static_cast<int>(sourceRGB.b) - 1))
    };

    const RGBLight mixedLight = {
        std::max(attenuatedLight.r, neighborLight.r),
        std::max(attenuatedLight.g, neighborLight.g),
        std::max(attenuatedLight.b, neighborLight.b)
    };

    if (mixedLight.r == neighborLight.r && mixedLight.g == neighborLight.g && mixedLight.b == neighborLight.b) {
        return false;
    }

    setBlockLightRGBAt(x, y, z, mixedLight);
    return true;
}

void Chunk::emitChangedLightsToNeighbors(const std::vector<LightPos> &changed) {
    std::unordered_map<ChunkPosition, std::list<PendingLight>> toEmit;
    toEmit.reserve(6);

    for (const auto [x, y, z] : changed) {
        if (!isBorderPosition(x, y, z)) continue;

        const uint8_t sunlightLvl = getSunLightLevelAt(x, y, z);
        const RGBLight rgbLvl = getBlockLightRGBLevelAt(x, y, z);

        if (sunlightLvl <= LightConstants::SUN_MIN + 1u && !rgbLvl.shouldPropagate(LightConstants::SUN_MIN)) {
            continue;
        }

        addLightToNeighborChunks(x, y, z, rgbLvl, sunlightLvl, toEmit);
    }

    if (!toEmit.empty()) {
        for (auto &[key, value] : toEmit) {
            m_pendingLightsForNeighbors[key].splice(m_pendingLightsForNeighbors[key].end(), value);
        }
    }
}

bool Chunk::isBorderPosition(const int x, const int y, const int z) {
    return x == -1 || x == static_cast<int>(SIZE) ||
           y == -1 || y == static_cast<int>(SIZE) ||
           z == -1 || z == static_cast<int>(SIZE);
}

void Chunk::addLightToNeighborChunks(int x, int y, int z, const RGBLight &rgb, uint8_t sunlight,
                                     std::unordered_map<ChunkPosition, std::list<PendingLight>> &toEmit) const {
    constexpr auto SIZE_INT = static_cast<int>(SIZE);

    if (x == -1) toEmit[{m_x - SIZE_INT, m_y, m_z}].emplace_back(SIZE_INT, y, z, rgb, sunlight);
    if (x == SIZE_INT) toEmit[{m_x + SIZE_INT, m_y, m_z}].emplace_back(-1, y, z, rgb, sunlight);
    if (y == -1) toEmit[{m_x, m_y - SIZE_INT, m_z}].emplace_back(x, SIZE_INT, z, rgb, sunlight);
    if (y == SIZE_INT) toEmit[{m_x, m_y + SIZE_INT, m_z}].emplace_back(x, -1, z, rgb, sunlight);
    if (z == -1) toEmit[{m_x, m_y, m_z - SIZE_INT}].emplace_back(x, y, SIZE_INT, rgb, sunlight);
    if (z == SIZE_INT) toEmit[{m_x, m_y, m_z + SIZE_INT}].emplace_back(x, y, -1, rgb, sunlight);
}

void Chunk::propagateSunLight(std::queue<uint32_t> &sunlightQueue) {
    // BFS algorithm
    while (!sunlightQueue.empty()) {
        const auto packed = sunlightQueue.front();
        sunlightQueue.pop();
        const auto [x, y, z] = unpackLightPos(packed);
        const uint8_t currentLightLevel = getSunLightLevelAt(x, y, z);
        if (currentLightLevel <= LightConstants::SUN_MIN) continue; // No more light to propagate

        for (auto [dx, dy, dz]: Block::s_faceOffset) {
            const int nx = x + dx;
            const int ny = y + dy;
            const int nz = z + dz;

            if (nx < -1 || ny < -1 || nz < -1 || nx >= SIZE + 1 || ny >= SIZE + 1 || nz >= SIZE + 1) continue;
            if (Block::isOpaque(getBlockType(nx, ny, nz))) continue;

            if (uint8_t neighborLightLevel = getSunLightLevelAt(nx, ny, nz);
                neighborLightLevel + 2u <= currentLightLevel) {
                neighborLightLevel = currentLightLevel - 1u;
                setSunLightLevelAt(nx, ny, nz, neighborLightLevel);
                sunlightQueue.emplace(packLightPos(nx, ny, nz));
            }
        }
    }

    emitBorderLights();
}

void Chunk::propagateBlockLight(std::queue<uint32_t> &blockLightQueue) {
    // BFS algorithm
    while (!blockLightQueue.empty()) {
        const auto packed = blockLightQueue.front();
        blockLightQueue.pop();
        const auto [x, y, z] = unpackLightPos(packed);
        const RGBLight currentRGB = getBlockLightRGBLevelAt(x, y, z);

        if (!currentRGB.shouldPropagate(LightConstants::SUN_MIN)) continue;

        for (auto [dx, dy, dz]: Block::s_faceOffset) {
            const int nx = x + dx;
            const int ny = y + dy;
            const int nz = z + dz;

            if (nx < -1 || ny < -1 || nz < -1 || nx >= SIZE + 1 || ny >= SIZE + 1 || nz >= SIZE + 1) continue;

            if (Block::isOpaque(getBlockType(nx, ny, nz))) continue;

            const RGBLight neighborLight = getBlockLightRGBLevelAt(nx, ny, nz);
            const RGBLight attenuatedLight = {
                static_cast<uint8_t>(std::max(static_cast<int>(LightConstants::SUN_MIN), static_cast<int>(currentRGB.r) - 1)),
                static_cast<uint8_t>(std::max(static_cast<int>(LightConstants::SUN_MIN), static_cast<int>(currentRGB.g) - 1)),
                static_cast<uint8_t>(std::max(static_cast<int>(LightConstants::SUN_MIN), static_cast<int>(currentRGB.b) - 1))
            };

            RGBLight mixedLight = neighborLight;
            bool shouldUpdate = false;

            if (attenuatedLight.r > neighborLight.r) {
                mixedLight.r = attenuatedLight.r;
                shouldUpdate = true;
            }
            if (attenuatedLight.g > neighborLight.g) {
                mixedLight.g = attenuatedLight.g;
                shouldUpdate = true;
            }
            if (attenuatedLight.b > neighborLight.b) {
                mixedLight.b = attenuatedLight.b;
                shouldUpdate = true;
            }

            if (shouldUpdate) {
                setBlockLightRGBAt(nx, ny, nz, mixedLight);
                blockLightQueue.push(packLightPos(nx, ny, nz));
            }
        }
    }

    emitBorderLights();
}

void Chunk::propagateBlockLightFrom(const int localX, const int localY, const int localZ) {
    std::queue<uint32_t> blockLightQueue;

    setBlockLightRGBAt(localX, localY, localZ, Block::getLightColor(getBlockType(localX, localY, localZ)));
    blockLightQueue.emplace(packLightPos(localX, localY, localZ));

    // BFS algorithm
    while (!blockLightQueue.empty()) {
        const auto packed = blockLightQueue.front();
        blockLightQueue.pop();
        const auto [x, y, z] = unpackLightPos(packed);
        const RGBLight currentRGB = getBlockLightRGBLevelAt(x, y, z);

        if (!currentRGB.shouldPropagate(LightConstants::SUN_MIN)) continue;

        for (auto [dx, dy, dz]: Block::s_faceOffset) {
            const int nx = x + dx;
            const int ny = y + dy;
            const int nz = z + dz;

            if (nx < -1 || ny < -1 || nz < -1 || nx >= SIZE + 1 || ny >= SIZE + 1 || nz >= SIZE + 1) continue;

            if (Block::isOpaque(getBlockType(nx, ny, nz))) continue;

            const RGBLight neighborLight = getBlockLightRGBLevelAt(nx, ny, nz);
            const RGBLight attenuatedLight = {
                static_cast<uint8_t>(std::max(static_cast<int>(LightConstants::SUN_MIN), static_cast<int>(currentRGB.r) - 1)),
                static_cast<uint8_t>(std::max(static_cast<int>(LightConstants::SUN_MIN), static_cast<int>(currentRGB.g) - 1)),
                static_cast<uint8_t>(std::max(static_cast<int>(LightConstants::SUN_MIN), static_cast<int>(currentRGB.b) - 1))
            };

            RGBLight mixedLight = neighborLight;
            bool shouldUpdate = false;

            if (attenuatedLight.r > neighborLight.r) {
                mixedLight.r = attenuatedLight.r;
                shouldUpdate = true;
            }
            if (attenuatedLight.g > neighborLight.g) {
                mixedLight.g = attenuatedLight.g;
                shouldUpdate = true;
            }
            if (attenuatedLight.b > neighborLight.b) {
                mixedLight.b = attenuatedLight.b;
                shouldUpdate = true;
            }

            if (shouldUpdate) {
                setBlockLightRGBAt(nx, ny, nz, mixedLight);
                blockLightQueue.push(packLightPos(nx, ny, nz));
            }
        }
    }

    emitBorderLights();
}

uint32_t Chunk::packLightPos(const int x, const int y, const int z) {
    static_assert(SIZE <= 63, "Chunk SIZE exceeds 63, cannot pack light position in 18 bits");
    constexpr unsigned int POS_MASK = 0x3F; // 6 bits
    return static_cast<uint32_t>(x + 1) & POS_MASK
           | (static_cast<uint32_t>(y + 1) & POS_MASK) << 6
           | (static_cast<uint32_t>(z + 1) & POS_MASK) << 12;
}

std::tuple<int, int, int> Chunk::unpackLightPos(const uint32_t v) {
    constexpr unsigned int POS_MASK = 0x3F; // 6 bits
    return std::tuple{
        static_cast<int>(v & POS_MASK) - 1,
        static_cast<int>(v >> 6 & POS_MASK) - 1,
        static_cast<int>(v >> 12 & POS_MASK) - 1
    };
}

uint8_t Chunk::getSunLightLevelAt(const int localX, const int localY, const int localZ) const {
    return m_lightLevels[index(localX + 1, localY + 1, localZ + 1)] & LightConstants::SUN_MASK;
}

void Chunk::setSunLightLevelAt(const int localX, const int localY, const int localZ, const uint8_t lightLevel) {
    const int idx = index(localX + 1, localY + 1, localZ + 1);
    const auto preservedRGB = static_cast<uint16_t>(m_lightLevels[idx] & (LightConstants::R_MASK | LightConstants::G_MASK | LightConstants::B_MASK));
    m_lightLevels[idx] = static_cast<uint16_t>(preservedRGB | LightConstants::packSun(lightLevel));
}

RGBLight Chunk::getBlockLightRGBLevelAt(const int localX, const int localY, const int localZ) const {
    const int idx = index(localX + 1, localY + 1, localZ + 1);
    const uint16_t packed = m_lightLevels[idx];
    return {
        LightConstants::unpackR(packed),
        LightConstants::unpackG(packed),
        LightConstants::unpackB(packed)
    };
}

void Chunk::setBlockLightRGBAt(const int localX, const int localY, const int localZ, const RGBLight &rgb) {
    const int idx = index(localX + 1, localY + 1, localZ + 1);
    const uint16_t sunlight = m_lightLevels[idx] & LightConstants::SUN_MASK;
    m_lightLevels[idx] = sunlight | LightConstants::packRGB(rgb.r, rgb.g, rgb.b);
}

bool Chunk::isBlockPresent(const int localX, const int localY, const int localZ) const {
    return m_blocks[index(localX + 1, localY + 1, localZ + 1)] != Block::BlockType::AIR;
}

bool Chunk::hasVisibleFaces() const {
    return !m_opaqueData.vertices.empty() || !m_waterData.vertices.empty();
}
