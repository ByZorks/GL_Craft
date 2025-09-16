#include "Chunk.h"

#include <algorithm>
#include <iostream>
#include <vector>

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
    m_surfaceFeatures.reserve(SIZE * SIZE * 0.25f);
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

void Chunk::generatePendingBlocks(std::list<PendingBlock> &blocks, MeshingResult &outResult) {
    for (const auto &[localX, localY, localZ, blockType]: blocks) {
        m_blocks[index(localX, localY, localZ)] = blockType;
    }

    propagateLight();
    generateNewMesh(outResult);
}

void Chunk::propagateLight() {
    constexpr unsigned int MIN_LIGHT_LEVEL = 1u;

    std::queue<uint32_t> sunlightQueue;
    std::queue<uint32_t> blockLightQueue;

    // Reset all light levels before a full recompute to avoid stale values
    std::ranges::fill(m_lightLevels, static_cast<uint16_t>(MIN_LIGHT_LEVEL));

    // First pass: vertical light propagation from the top
    for (int localX = 0; localX < SIZE + 2; localX++) {
        const int adjustedX = localX - 1;

        for (int localZ = 0; localZ < SIZE + 2; localZ++) {
            const int adjustedZ = localZ - 1;

            TerrainGenerator::NoiseValues noises;
            noises.computeHeightNoises(m_x + adjustedX, m_z + adjustedZ);
            const bool inDirectSunlight = !(TerrainGenerator::getHeight(noises) > m_y + SIZE ||
                                            TerrainGenerator::getSeaLevel() > m_y + SIZE);

            unsigned int currentLightLevel = inDirectSunlight ? 15u : MIN_LIGHT_LEVEL;

            for (int localY = SIZE + 1; localY >= 0; localY--) {
                const int adjustedY = localY - 1;

                const Block::BlockType blockType = getBlockType(adjustedX, adjustedY, adjustedZ);

                if (Block::isLightEmitter(blockType)) {
                    setBlockLightRGBAt(adjustedX, adjustedY, adjustedZ, Block::getLightColor(blockType));
                    blockLightQueue.emplace(packLightPos(adjustedX, adjustedY, adjustedZ));
                }

                if (!inDirectSunlight) continue;

                if (Block::isTransparent(blockType)) {
                    setSunLightLevelAt(adjustedX, adjustedY, adjustedZ, currentLightLevel);
                    if (blockType != Block::BlockType::AIR) {
                        // use signed int to avoid unsigned underflow when subtracting
                        int signedLevel = static_cast<int>(currentLightLevel) - 2;
                        signedLevel = std::max(static_cast<int>(MIN_LIGHT_LEVEL), signedLevel);
                        currentLightLevel = static_cast<unsigned int>(signedLevel);
                    }
                    if (currentLightLevel > MIN_LIGHT_LEVEL) {
                        sunlightQueue.emplace(packLightPos(adjustedX, adjustedY, adjustedZ));
                    }
                } else {
                    setSunLightLevelAt(adjustedX, adjustedY, adjustedZ, MIN_LIGHT_LEVEL);
                    currentLightLevel = MIN_LIGHT_LEVEL;
                }
            }
        }
    }

    // Second pass: horizontal sunlight propagation (BFS algorithm)
    while (!sunlightQueue.empty()) {
        const auto packed = sunlightQueue.front();
        sunlightQueue.pop();
        const auto [x, y, z] = unpackLightPos(packed);
        const uint8_t currentLightLevel = getSunLightLevelAt(x, y, z);
        if (currentLightLevel <= MIN_LIGHT_LEVEL) continue; // No more light to propagate

        for (auto [dx, dy, dz]: Block::s_faceOffset) {
            const int nx = x + dx;
            const int ny = y + dy;
            const int nz = z + dz;

            if (nx < -1 || ny < -1 || nz < -1 || nx >= SIZE + 1 || ny >= SIZE + 1 || nz >= SIZE + 1) continue;

            if (const Block::BlockType neighborBlockType = getBlockType(nx, ny, nz);
                Block::isOpaque(neighborBlockType)) continue;

            if (uint8_t neighborLightLevel = getSunLightLevelAt(nx, ny, nz);
                neighborLightLevel + 2u <= currentLightLevel) {
                neighborLightLevel = currentLightLevel - 1u;
                setSunLightLevelAt(nx, ny, nz, neighborLightLevel);
                sunlightQueue.emplace(packLightPos(nx, ny, nz));
            }
        }
    }

    // Third pass: propagate block light from light-emitting blocks
    while (!blockLightQueue.empty()) {
        const auto packed = blockLightQueue.front();
        blockLightQueue.pop();
        const auto [x, y, z] = unpackLightPos(packed);
        const RGBLight currentRGB = getBlockLightRGBLevelAt(x, y, z);

        if (!currentRGB.shouldPropagate(MIN_LIGHT_LEVEL)) continue;

        for (auto [dx, dy, dz]: Block::s_faceOffset) {
            const int nx = x + dx;
            const int ny = y + dy;
            const int nz = z + dz;

            if (nx < -1 || ny < -1 || nz < -1 || nx >= SIZE + 1 || ny >= SIZE + 1 || nz >= SIZE + 1) continue;

            if (const Block::BlockType neighborBlockType = getBlockType(nx, ny, nz);
                Block::isOpaque(neighborBlockType)) continue;

            const RGBLight neighborLight = getBlockLightRGBLevelAt(nx, ny, nz);
            const RGBLight attenuatedLight = {
                static_cast<uint8_t>(std::max(static_cast<int>(MIN_LIGHT_LEVEL), static_cast<int>(currentRGB.r) - 1)),
                static_cast<uint8_t>(std::max(static_cast<int>(MIN_LIGHT_LEVEL), static_cast<int>(currentRGB.g) - 1)),
                static_cast<uint8_t>(std::max(static_cast<int>(MIN_LIGHT_LEVEL), static_cast<int>(currentRGB.b) - 1))
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
                if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE && nz >= 0 && nz < SIZE) {
                    blockLightQueue.push(packLightPos(nx, ny, nz));
                }
            }
        }
    }

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

void Chunk::emitBorderLights() {
    constexpr uint8_t MIN_LIGHT_LEVEL = 1u;
    auto emitFace = [&](const int faceAxis, const bool positive){
        const int sample = positive ? static_cast<int>(SIZE) - 1 : 0;
        const int emitCoord = positive ? -1 : static_cast<int>(SIZE);
        const int offset = (positive ? 1 : -1) * static_cast<int>(SIZE);
        std::list<PendingLight> batch;

        if (faceAxis == 0) { // X faces
            for (int y = 0; y < static_cast<int>(SIZE); ++y) {
                for (int z = 0; z < static_cast<int>(SIZE); ++z) {
                    const uint8_t sunLvl = getSunLightLevelAt(sample, y, z);
                    const RGBLight blockLight = getBlockLightRGBLevelAt(sample, y, z);
                    const uint8_t blockLvl = blockLight.getEffectiveLevel();
                    if (sunLvl <= MIN_LIGHT_LEVEL && blockLvl <= MIN_LIGHT_LEVEL) continue;

                    batch.emplace_back(emitCoord, y, z, blockLight, sunLvl);
                }
            }
            if (!batch.empty()) {
                const ChunkPosition key(m_x + offset, m_y, m_z);
                m_pendingLightsForNeighbors[key].splice(m_pendingLightsForNeighbors[key].end(), batch);
            }
        } else if (faceAxis == 1) { // Y faces
            for (int x = 0; x < static_cast<int>(SIZE); ++x) {
                for (int z = 0; z < static_cast<int>(SIZE); ++z) {
                    const uint8_t sunLvl = getSunLightLevelAt(x, sample, z);
                    const RGBLight blockLight = getBlockLightRGBLevelAt(x, sample, z);
                    const uint8_t blockLvl = blockLight.getEffectiveLevel();
                    if (sunLvl <= MIN_LIGHT_LEVEL && blockLvl <= MIN_LIGHT_LEVEL) continue;

                    batch.emplace_back(x, emitCoord, z, blockLight, sunLvl);
                }
            }
            if (!batch.empty()) {
                const ChunkPosition key(m_x, m_y + offset, m_z);
                m_pendingLightsForNeighbors[key].splice(m_pendingLightsForNeighbors[key].end(), batch);
            }
        } else { // Z faces
            for (int x = 0; x < static_cast<int>(SIZE); ++x) {
                for (int y = 0; y < static_cast<int>(SIZE); ++y) {
                    const uint8_t sunLvl = getSunLightLevelAt(x, y, sample);
                    const RGBLight blockLight = getBlockLightRGBLevelAt(x, y, sample);
                    const uint8_t blockLvl = blockLight.getEffectiveLevel();
                    if (sunLvl <= MIN_LIGHT_LEVEL && blockLvl <= MIN_LIGHT_LEVEL) continue;

                    batch.emplace_back(x, y, emitCoord, blockLight, sunLvl);
                }
            }
            if (!batch.empty()) {
                const ChunkPosition key(m_x, m_y, m_z + offset);
                m_pendingLightsForNeighbors[key].splice(m_pendingLightsForNeighbors[key].end(), batch);
            }
        }
    };

    emitFace(0, false); // X-
    emitFace(0, true);  // X+
    emitFace(1, false); // Y-
    emitFace(1, true);  // Y+
    emitFace(2, false); // Z-
    emitFace(2, true);  // Z+
}

void Chunk::transferPendingBlocksToWorld(WorldManager &world) {
    if (m_pendingBlocksForNeighbors.empty()) return;
    world.addPendingBlocks(m_pendingBlocksForNeighbors);
}

void Chunk::transferPendingLightsToWorld(WorldManager &world) {
    if (m_pendingLightsForNeighbors.empty()) return;
    world.addPendingLights(m_pendingLightsForNeighbors);
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

    propagateLight();
    generateNewMesh(outResult);
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

void Chunk::processColumn(const int worldX, const int worldZ, const int localX, const int localZ,
                          const std::span<float> &tunnelCavesNoises, const std::span<float> &largeCavesNoises) {
    TerrainGenerator::NoiseValues noises;
    noises.computeHeightNoises(worldX, worldZ);

    const int columnHeight = TerrainGenerator::getHeight(noises);

    // Early exit for aerial chunks
    if (columnHeight < m_y - static_cast<int>(SIZE)) return;

    noises.computeRemainingNoises(worldX, worldZ);
    const Biome biome = TerrainGenerator::getBiome(noises, worldX, worldZ);
    const ChunkPosition position(m_x, m_y, m_z);

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

template<typename NoiseFunction>
void Chunk::getDownsampledNoises(const int factor, std::span<float> &outNoises, NoiseFunction noiseFunction) const {
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
    return m_lightLevels[index(localX + 1, localY + 1, localZ + 1)] & 0x0F;
}

void Chunk::setSunLightLevelAt(const int localX, const int localY, const int localZ, const uint8_t lightLevel) {
    const int idx = index(localX + 1, localY + 1, localZ + 1);
    m_lightLevels[idx] = static_cast<uint16_t>(m_lightLevels[idx] & 0xFFF0 | lightLevel & 0x0F);
}

RGBLight Chunk::getBlockLightRGBLevelAt(const int localX, const int localY, const int localZ) const {
    const int idx = index(localX + 1, localY + 1, localZ + 1);
    const uint16_t packed = m_lightLevels[idx];
    return {
        static_cast<uint8_t>(packed >> 4 & 0xF),
        static_cast<uint8_t>(packed >> 8 & 0xF),
        static_cast<uint8_t>(packed >> 12 & 0xF)
    };
}

void Chunk::setBlockLightRGBAt(const int localX, const int localY, const int localZ, const RGBLight &rgb) {
    const int idx = index(localX + 1, localY + 1, localZ + 1);
    const uint16_t sunlight = m_lightLevels[idx] & 0x0F;
    m_lightLevels[idx] = sunlight |
                        static_cast<uint16_t>(rgb.r & 0xF) << 4 |
                        static_cast<uint16_t>(rgb.g & 0xF) << 8 |
                        static_cast<uint16_t>(rgb.b & 0xF) << 12;
}

bool Chunk::isBlockPresent(const int localX, const int localY, const int localZ) const {
    return m_blocks[index(localX + 1, localY + 1, localZ + 1)] != Block::BlockType::AIR;
}

bool Chunk::hasVisibleFaces() const {
    return !m_opaqueData.vertices.empty() || !m_waterData.vertices.empty();
}

void Chunk::generatePendingLights(std::list<PendingLight> &lights, MeshingResult &outResult) {
    if (lights.empty()) return;
    constexpr uint8_t MIN_LIGHT_LEVEL = 1u;

    std::queue<uint32_t> sunLightQueue;
    std::queue<uint32_t> blockLightQueue;

    struct Pos { int x, y, z; };
    std::vector<Pos> changed;
    changed.reserve(lights.size() * 8);

    // Initial light sources
    for (const auto &[localX, localY, localZ, blockLight, sunlight] : lights) {
        if (localX < -1 || localY < -1 || localZ < -1 || localX > static_cast<int>(SIZE) || localY > static_cast<int>(SIZE) || localZ > static_cast<int>(SIZE)) continue;
        if (Block::isOpaque(getBlockType(localX, localY, localZ))) continue;

        bool anyChange = false;
        if (sunlight > getSunLightLevelAt(localX, localY, localZ)) {
            setSunLightLevelAt(localX, localY, localZ, sunlight);
            if (sunlight > MIN_LIGHT_LEVEL) sunLightQueue.emplace(packLightPos(localX, localY, localZ));
            anyChange = true;
        }

        const RGBLight currentRGB = getBlockLightRGBLevelAt(localX, localY, localZ);
        RGBLight newRGB = currentRGB;
        bool rgbChanged = false;

        if (blockLight.r > currentRGB.r) {
            newRGB.r = blockLight.r;
            rgbChanged = true;
        }
        if (blockLight.g > currentRGB.g) {
            newRGB.g = blockLight.g;
            rgbChanged = true;
        }
        if (blockLight.b > currentRGB.b) {
            newRGB.b = blockLight.b;
            rgbChanged = true;
        }

        if (rgbChanged) {
            setBlockLightRGBAt(localX, localY, localZ, newRGB);
            if (newRGB.shouldPropagate(MIN_LIGHT_LEVEL)) blockLightQueue.emplace(packLightPos(localX, localY, localZ));
            anyChange = true;
        }

        if (anyChange) changed.emplace_back(localX, localY, localZ);
    }

    // BFS propagation for sunlight
    while (!sunLightQueue.empty()) {
        const auto n = sunLightQueue.front(); sunLightQueue.pop();
        const auto [x, y, z] = unpackLightPos(n);
        const uint8_t currentLightLevel = getSunLightLevelAt(x, y, z);
        if (currentLightLevel <= MIN_LIGHT_LEVEL) continue;

        for (auto [dx, dy, dz] : Block::s_faceOffset) {
            const int nx = x + dx, ny = y + dy, nz = z + dz;
            if (nx < -1 || ny < -1 || nz < -1 || nx >= static_cast<int>(SIZE) + 1 || ny >= static_cast<int>(SIZE) + 1 || nz >= static_cast<int>(SIZE) + 1) continue;
            if (Block::isOpaque(getBlockType(nx, ny, nz))) continue;

            if (const uint8_t neighborLightLevel = getSunLightLevelAt(nx, ny, nz);
                neighborLightLevel + 2u <= currentLightLevel) {

                if (const auto newLevel = static_cast<uint8_t>(currentLightLevel - 1u);
                    newLevel > neighborLightLevel) {
                    setSunLightLevelAt(nx, ny, nz, newLevel);
                    changed.emplace_back(nx,ny,nz);
                    sunLightQueue.emplace(packLightPos(nx,ny,nz));
                }
            }
        }
    }

    // BFS propagation for block light
    while (!blockLightQueue.empty()) {
        const auto n = blockLightQueue.front(); blockLightQueue.pop();
        const auto [x, y, z] = unpackLightPos(n);
        const RGBLight currentRGB = getBlockLightRGBLevelAt(x, y, z);
        if (!currentRGB.shouldPropagate(MIN_LIGHT_LEVEL)) continue;

        for (auto [dx, dy, dz] : Block::s_faceOffset) {
            const int nx = x + dx, ny = y + dy, nz = z + dz;
            if (nx < -1 || ny < -1 || nz < -1 || nx >= static_cast<int>(SIZE) + 1 || ny >= static_cast<int>(SIZE) + 1 || nz >= static_cast<int>(SIZE) + 1) continue;
            if (Block::isOpaque(getBlockType(nx, ny, nz))) continue;

            const RGBLight neighborLight = getBlockLightRGBLevelAt(nx, ny, nz);
            const RGBLight attenuatedLight = {
                static_cast<uint8_t>(std::max(0, static_cast<int>(currentRGB.r) - 1)),
                static_cast<uint8_t>(std::max(0, static_cast<int>(currentRGB.g) - 1)),
                static_cast<uint8_t>(std::max(0, static_cast<int>(currentRGB.b) - 1))
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
                if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE && nz >= 0 && nz < SIZE) {
                    setBlockLightRGBAt(nx, ny, nz, mixedLight);
                    blockLightQueue.emplace(packLightPos(nx, ny, nz));
                    changed.emplace_back(nx,ny,nz);
                }
            }
        }
    }

    if (changed.empty()) return;

    // Emit only changed border cells to neighbors
    std::unordered_map<ChunkPosition, std::list<PendingLight>> toEmit;
    toEmit.reserve(6);
    for (const auto [x, y, z] : changed) {
        const bool borderX = x == -1 || x == static_cast<int>(SIZE);
        const bool borderY = y == -1 || y == static_cast<int>(SIZE);
        const bool borderZ = z == -1 || z == static_cast<int>(SIZE);
        if (!borderX && !borderY && !borderZ) continue;

        const uint8_t sunlightLvl = getSunLightLevelAt(x, y, z);
        const RGBLight rgbLvl = getBlockLightRGBLevelAt(x, y, z);

        if (sunlightLvl <= MIN_LIGHT_LEVEL && !rgbLvl.shouldPropagate(MIN_LIGHT_LEVEL)) continue;

        if (x == -1)  toEmit[{m_x - static_cast<int>(SIZE), m_y, m_z}].emplace_back(static_cast<int>(SIZE), y, z, rgbLvl, sunlightLvl);
        if (x == static_cast<int>(SIZE)) toEmit[{m_x + static_cast<int>(SIZE), m_y, m_z}].emplace_back(-1, y, z, rgbLvl, sunlightLvl);
        if (y == -1)  toEmit[{m_x, m_y - static_cast<int>(SIZE), m_z}].emplace_back(x, static_cast<int>(SIZE), z, rgbLvl, sunlightLvl);
        if (y == static_cast<int>(SIZE)) toEmit[{m_x, m_y + static_cast<int>(SIZE), m_z}].emplace_back(x, -1, z, rgbLvl, sunlightLvl);
        if (z == -1)  toEmit[{m_x, m_y, m_z - static_cast<int>(SIZE)}].emplace_back(x, y, static_cast<int>(SIZE), rgbLvl, sunlightLvl);
        if (z == static_cast<int>(SIZE)) toEmit[{m_x, m_y, m_z + static_cast<int>(SIZE)}].emplace_back(x, y, -1, rgbLvl, sunlightLvl);
    }

    if (!toEmit.empty()) {
        for (auto &[key, value] : toEmit) {
            m_pendingLightsForNeighbors[key].splice(m_pendingLightsForNeighbors[key].end(), value);
        }
    }

    outResult.needIndirectRendererUpdate = true;
    outResult.needInstanceUpdate = false;
    generateNewMesh(outResult);
}
