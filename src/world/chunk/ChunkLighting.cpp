//
// Created by david on 20/09/2025.
//

#include "ChunkLighting.h"

#include "Chunk.h"
#include "LightConstants.h"
#include "MeshManager.h"

ChunkLighting::ChunkLighting(Chunk &chunk) : m_chunk(chunk) {
}

void ChunkLighting::propagateLight() const {
    // Reset all light levels before a full recompute to avoid stale values
    std::ranges::fill(m_chunk.m_lightLevels, static_cast<uint16_t>(LightConstants::SUN_MIN));

    std::queue<uint32_t> sunlightQueue;
    std::queue<uint32_t> blocklightQueue;

    // First pass: vertical light propagation from the top
    for (int localX = 0; localX < Chunk::SIZE + 2; localX++) {
        const int adjustedX = localX - 1;

        for (int localZ = 0; localZ < Chunk::SIZE + 2; localZ++) {
            const int adjustedZ = localZ - 1;

            TerrainGenerator::NoiseValues noises;
            noises.computeHeightNoises(m_chunk.m_x + adjustedX, m_chunk.m_z + adjustedZ);
            const bool inDirectSunlight = !(TerrainGenerator::getHeight(noises) > m_chunk.m_y + Chunk::SIZE ||
                                            TerrainGenerator::getSeaLevel() > m_chunk.m_y + Chunk::SIZE);

            unsigned int currentLightLevel = inDirectSunlight ? 15u : LightConstants::SUN_MIN;

            for (int localY = Chunk::SIZE + 1; localY >= 0; localY--) {
                const int adjustedY = localY - 1;

                const Block::BlockType blockType = m_chunk.getBlockType(adjustedX, adjustedY, adjustedZ);

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

void ChunkLighting::generatePendingLights(const std::list<PendingLight> &lights, MeshingResult &outResult) const {
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
    m_chunk.generateNewMesh(outResult);
}

void ChunkLighting::emitBorderLights() const {
        auto emitXFace = [&](const bool positive) {
        const int sample = positive ? static_cast<int>(Chunk::SIZE) - 1 : 0;
        const int emitCoord = positive ? -1 : static_cast<int>(Chunk::SIZE);
        const int offset = (positive ? 1 : -1) * static_cast<int>(Chunk::SIZE);
        std::list<PendingLight> batch;

        for (int y = 0; y < static_cast<int>(Chunk::SIZE); ++y) {
            for (int z = 0; z < static_cast<int>(Chunk::SIZE); ++z) {
                const uint8_t sunLvl = getSunLightLevelAt(sample, y, z);
                const RGBLight blockLight = getBlockLightRGBLevelAt(sample, y, z);
                if (sunLvl <= LightConstants::SUN_MIN + 1u && !blockLight.shouldPropagate(LightConstants::SUN_MIN)) {
                    continue;
                }
                batch.emplace_back(emitCoord, y, z, blockLight, sunLvl);
            }
        }
        if (!batch.empty()) {
            const ChunkPosition key{m_chunk.m_x + offset, m_chunk.m_y, m_chunk.m_z};
            m_chunk.m_pendingLightsForNeighbors[key].splice(m_chunk.m_pendingLightsForNeighbors[key].end(), batch);
        }
    };

    auto emitYFace = [&](const bool positive) {
        const int sample = positive ? static_cast<int>(Chunk::SIZE) - 1 : 0;
        const int emitCoord = positive ? -1 : static_cast<int>(Chunk::SIZE);
        const int offset = (positive ? 1 : -1) * static_cast<int>(Chunk::SIZE);
        std::list<PendingLight> batch;

        for (int x = 0; x < static_cast<int>(Chunk::SIZE); ++x) {
            for (int z = 0; z < static_cast<int>(Chunk::SIZE); ++z) {
                const uint8_t sunLvl = getSunLightLevelAt(x, sample, z);
                const RGBLight blockLight = getBlockLightRGBLevelAt(x, sample, z);
                if (sunLvl <= LightConstants::SUN_MIN + 1u && !blockLight.shouldPropagate(LightConstants::SUN_MIN)) {
                    continue;
                }
                batch.emplace_back(x, emitCoord, z, blockLight, sunLvl);
            }
        }
        if (!batch.empty()) {
            const ChunkPosition key{m_chunk.m_x, m_chunk.m_y + offset, m_chunk.m_z};
            m_chunk.m_pendingLightsForNeighbors[key].splice(m_chunk.m_pendingLightsForNeighbors[key].end(), batch);
        }
    };

    auto emitZFace = [&](const bool positive) {
        const int sample = positive ? static_cast<int>(Chunk::SIZE) - 1 : 0;
        const int emitCoord = positive ? -1 : static_cast<int>(Chunk::SIZE);
        const int offset = (positive ? 1 : -1) * static_cast<int>(Chunk::SIZE);
        std::list<PendingLight> batch;

        for (int x = 0; x < static_cast<int>(Chunk::SIZE); ++x) {
            for (int y = 0; y < static_cast<int>(Chunk::SIZE); ++y) {
                const uint8_t sunLvl = getSunLightLevelAt(x, y, sample);
                const RGBLight blockLight = getBlockLightRGBLevelAt(x, y, sample);
                if (sunLvl <= LightConstants::SUN_MIN + 1u && !blockLight.shouldPropagate(LightConstants::SUN_MIN)) {
                    continue;
                }
                batch.emplace_back(x, y, emitCoord, blockLight, sunLvl);
            }
        }
        if (!batch.empty()) {
            const ChunkPosition key{m_chunk.m_x, m_chunk.m_y, m_chunk.m_z + offset};
            m_chunk.m_pendingLightsForNeighbors[key].splice(m_chunk.m_pendingLightsForNeighbors[key].end(), batch);
        }
    };

    emitXFace(false); // X-
    emitXFace(true);  // X+
    emitYFace(false); // Y-
    emitYFace(true);  // Y+
    emitZFace(false); // Z-
    emitZFace(true);  // Z+
}

void ChunkLighting::processInitialLightSources(const std::list<PendingLight> &lights,
    std::queue<uint32_t> &sunLightQueue, std::queue<uint32_t> &blockLightQueue, std::vector<LightPos> &changed) const {
    for (const auto &[localX, localY, localZ, blockLight, sunlight] : lights) {
        if (!isValidLightPosition(localX, localY, localZ)) continue;
        if (Block::isOpaque(m_chunk.getBlockType(localX, localY, localZ))) continue;

        const bool sunChanged = updateSunLight(localX, localY, localZ, sunlight, sunLightQueue);
        const bool rgbChanged = updateBlockLight(localX, localY, localZ, blockLight, blockLightQueue);

        if (sunChanged || rgbChanged) {
            changed.emplace_back(localX, localY, localZ);
        }
    }
}

bool ChunkLighting::isValidLightPosition(const int x, const int y, const int z) {
    return x >= -1 && y >= -1 && z >= -1 &&
           x <= static_cast<int>(Chunk::SIZE) &&
           y <= static_cast<int>(Chunk::SIZE) &&
           z <= static_cast<int>(Chunk::SIZE);
}

bool ChunkLighting::updateSunLight(const int x, const int y, const int z, const uint8_t sunlight, std::queue<uint32_t> &queue) const {
    if (sunlight <= getSunLightLevelAt(x, y, z)) return false;

    setSunLightLevelAt(x, y, z, sunlight);
    if (sunlight > LightConstants::SUN_MIN) {
        queue.emplace(packLightPos(x, y, z));
    }
    return true;
}

bool ChunkLighting::updateBlockLight(const int x, const int y, const int z, const RGBLight &blockLight, std::queue<uint32_t> &queue) const {
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

void ChunkLighting::propagateSunLightBFS(std::queue<uint32_t> &sunLightQueue, std::vector<LightPos> &changed) const {
    while (!sunLightQueue.empty()) {
        const auto [x, y, z] = unpackLightPos(sunLightQueue.front());
        sunLightQueue.pop();

        const uint8_t currentLightLevel = getSunLightLevelAt(x, y, z);
        if (currentLightLevel <= LightConstants::SUN_MIN) continue;

        propagateSunLightToNeighbors(x, y, z, currentLightLevel, sunLightQueue, changed);
    }
}

void ChunkLighting::propagateSunLightToNeighbors(const int x, const int y, const int z, const uint8_t currentLevel,
    std::queue<uint32_t> &queue, std::vector<LightPos> &changed) const {
    for (auto [dx, dy, dz] : Block::s_faceOffset) {
        const int nx = x + dx;
        const int ny = y + dy;
        const int nz = z + dz;

        if (!isValidLightPosition(nx, ny, nz)) continue;
        if (Block::isOpaque(m_chunk.getBlockType(nx, ny, nz))) continue;

        const uint8_t neighborLevel = getSunLightLevelAt(nx, ny, nz);
        const auto newLevel = static_cast<uint8_t>(currentLevel - 1u);

        if (neighborLevel + 2u <= currentLevel && newLevel > neighborLevel) {
            setSunLightLevelAt(nx, ny, nz, newLevel);
            changed.emplace_back(nx, ny, nz);
            queue.emplace(packLightPos(nx, ny, nz));
        }
    }
}

void ChunkLighting::propagateBlockLightBFS(std::queue<uint32_t> &blockLightQueue, std::vector<LightPos> &changed) const {
    while (!blockLightQueue.empty()) {
        const auto [x, y, z] = unpackLightPos(blockLightQueue.front());
        blockLightQueue.pop();

        const RGBLight currentRGB = getBlockLightRGBLevelAt(x, y, z);
        if (!currentRGB.shouldPropagate(LightConstants::SUN_MIN)) continue;

        propagateBlockLightToNeighbors(x, y, z, currentRGB, blockLightQueue, changed);
    }
}

void ChunkLighting::propagateBlockLightToNeighbors(const int x, const int y, const int z, const RGBLight &currentRGB,
    std::queue<uint32_t> &queue, std::vector<LightPos> &changed) const {
    for (auto [dx, dy, dz] : Block::s_faceOffset) {
        const int nx = x + dx;
        const int ny = y + dy;
        const int nz = z + dz;

        if (!isValidLightPosition(nx, ny, nz)) continue;
        if (Block::isOpaque(m_chunk.getBlockType(nx, ny, nz))) continue;

        if (tryUpdateBlockLightAt(nx, ny, nz, currentRGB)) {
            queue.emplace(packLightPos(nx, ny, nz));
            changed.emplace_back(nx, ny, nz);
        }
    }
}

bool ChunkLighting::tryUpdateBlockLightAt(const int x, const int y, const int z, const RGBLight &sourceRGB) const {
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

void ChunkLighting::emitChangedLightsToNeighbors(const std::vector<LightPos> &changed) const {
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
            m_chunk.m_pendingLightsForNeighbors[key].splice(m_chunk.m_pendingLightsForNeighbors[key].end(), value);
        }
    }
}

bool ChunkLighting::isBorderPosition(const int x, const int y, const int z) {
    return x == -1 || x == static_cast<int>(Chunk::SIZE) ||
           y == -1 || y == static_cast<int>(Chunk::SIZE) ||
           z == -1 || z == static_cast<int>(Chunk::SIZE);
}

void ChunkLighting::addLightToNeighborChunks(const int x, const int y, const int z, const RGBLight &rgb, const uint8_t sunlight,
    std::unordered_map<ChunkPosition, std::list<PendingLight>> &toEmit) const {
    constexpr auto SIZE_INT = static_cast<int>(Chunk::SIZE);

    if (x == -1) toEmit[{m_chunk.m_x - SIZE_INT, m_chunk.m_y, m_chunk.m_z}].emplace_back(SIZE_INT, y, z, rgb, sunlight);
    if (x == SIZE_INT) toEmit[{m_chunk.m_x + SIZE_INT, m_chunk.m_y, m_chunk.m_z}].emplace_back(-1, y, z, rgb, sunlight);
    if (y == -1) toEmit[{m_chunk.m_x, m_chunk.m_y - SIZE_INT, m_chunk.m_z}].emplace_back(x, SIZE_INT, z, rgb, sunlight);
    if (y == SIZE_INT) toEmit[{m_chunk.m_x, m_chunk.m_y + SIZE_INT, m_chunk.m_z}].emplace_back(x, -1, z, rgb, sunlight);
    if (z == -1) toEmit[{m_chunk.m_x, m_chunk.m_y, m_chunk.m_z - SIZE_INT}].emplace_back(x, y, SIZE_INT, rgb, sunlight);
    if (z == SIZE_INT) toEmit[{m_chunk.m_x, m_chunk.m_y, m_chunk.m_z + SIZE_INT}].emplace_back(x, y, -1, rgb, sunlight);
}

void ChunkLighting::propagateSunLight(std::queue<uint32_t> &sunlightQueue) const {
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

            if (nx < -1 || ny < -1 || nz < -1 || nx >= Chunk::SIZE + 1 || ny >= Chunk::SIZE + 1 || nz >= Chunk::SIZE + 1) continue;
            if (Block::isOpaque(m_chunk.getBlockType(nx, ny, nz))) continue;

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

void ChunkLighting::propagateBlockLight(std::queue<uint32_t> &blockLightQueue) const {
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

            if (nx < -1 || ny < -1 || nz < -1 || nx >= Chunk::SIZE + 1 || ny >= Chunk::SIZE + 1 || nz >= Chunk::SIZE + 1) continue;

            if (Block::isOpaque(m_chunk.getBlockType(nx, ny, nz))) continue;

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

void ChunkLighting::propagateBlockLightFrom(const int localX, const int localY, const int localZ) const {
    std::queue<uint32_t> blockLightQueue;

    setBlockLightRGBAt(localX, localY, localZ, Block::getLightColor(m_chunk.getBlockType(localX, localY, localZ)));
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

            if (nx < -1 || ny < -1 || nz < -1 || nx >= Chunk::SIZE + 1 || ny >= Chunk::SIZE + 1 || nz >= Chunk::SIZE + 1) continue;

            if (Block::isOpaque(m_chunk.getBlockType(nx, ny, nz))) continue;

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

uint32_t ChunkLighting::packLightPos(const int x, const int y, const int z) {
    static_assert(Chunk::SIZE <= 63, "Chunk SIZE exceeds 63, cannot pack light position in 18 bits");
    constexpr unsigned int POS_MASK = 0x3F; // 6 bits
    return static_cast<uint32_t>(x + 1) & POS_MASK
           | (static_cast<uint32_t>(y + 1) & POS_MASK) << 6
           | (static_cast<uint32_t>(z + 1) & POS_MASK) << 12;
}

std::tuple<int, int, int> ChunkLighting::unpackLightPos(const uint32_t v) {
    constexpr unsigned int POS_MASK = 0x3F; // 6 bits
    return std::tuple{
        static_cast<int>(v & POS_MASK) - 1,
        static_cast<int>(v >> 6 & POS_MASK) - 1,
        static_cast<int>(v >> 12 & POS_MASK) - 1
    };
}

uint8_t ChunkLighting::getSunLightLevelAt(const int localX, const int localY, const int localZ) const {
    return m_chunk.m_lightLevels[m_chunk.index(localX + 1, localY + 1, localZ + 1)] & LightConstants::SUN_MASK;
}

void ChunkLighting::setSunLightLevelAt(const int localX, const int localY, const int localZ, const uint8_t lightLevel) const {
    const int idx = m_chunk.index(localX + 1, localY + 1, localZ + 1);
    const auto preservedRGB = static_cast<uint16_t>(m_chunk.m_lightLevels[idx] & (LightConstants::R_MASK | LightConstants::G_MASK | LightConstants::B_MASK));
    m_chunk.m_lightLevels[idx] = static_cast<uint16_t>(preservedRGB | LightConstants::packSun(lightLevel));
}

RGBLight ChunkLighting::getBlockLightRGBLevelAt(const int localX, const int localY, const int localZ) const {
    const int idx = m_chunk.index(localX + 1, localY + 1, localZ + 1);
    const uint16_t packed = m_chunk.m_lightLevels[idx];
    return {
        LightConstants::unpackR(packed),
        LightConstants::unpackG(packed),
        LightConstants::unpackB(packed)
    };
}

void ChunkLighting::setBlockLightRGBAt(const int localX, const int localY, const int localZ, const RGBLight &rgb) const {
    const int idx = m_chunk.index(localX + 1, localY + 1, localZ + 1);
    const uint16_t sunlight = m_chunk.m_lightLevels[idx] & LightConstants::SUN_MASK;
    m_chunk.m_lightLevels[idx] = sunlight | LightConstants::packRGB(rgb.r, rgb.g, rgb.b);
}
