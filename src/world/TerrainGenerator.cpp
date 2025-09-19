#include "TerrainGenerator.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <utility>

#include "chunk/Chunk.h"

int TerrainGenerator::getHeight(const NoiseValues &noises) {
    const int baseHeight = getBaseLevel(noises);

    // 2D noise generation for terrain height
    const float normalizedNoise = (noises.terrain + 1.0f) / 2.0f;

    const float terrainShape = normalizedNoise * normalizedNoise * normalizedNoise * normalizedNoise;
    const float columnHeight = std::floor(static_cast<float>(baseHeight) + terrainShape * HEIGHT_MULTIPLIER);

    return static_cast<int>(columnHeight);
}

bool TerrainGenerator::isCave(const ChunkPosition &position, const int worldX, const int worldY, const int worldZ,
                              const int columnHeight, const std::span<const float> &largeCavesNoises,
                              const std::span<const float> &tunnelCavesNoises) {
    if (worldY <= 1 || worldY > HEIGHT_MULTIPLIER || (worldY >= columnHeight && columnHeight < SEA_LEVEL)) return false;

    // Up sample the 3D noise values
    constexpr int step8 = 8;
    constexpr int gridSizeX8 = (Chunk::SIZE + 2 + step8 - 1) / step8 + 1;
    constexpr int gridSizeY8 = gridSizeX8;
    constexpr int gridSizeZ8 = gridSizeX8;
    const float largeCave = trilinearInterpolation(largeCavesNoises, position, gridSizeX8, gridSizeY8, gridSizeZ8,
                                                   worldX, worldY, worldZ, step8);

    constexpr float cheeseThreshold = 0.6f;
    const bool isCheeseCave = largeCave > cheeseThreshold;

    // Tunnel caves only if not already a cheese cave
    if (!isCheeseCave) {
        bool isTunnel = false;
        constexpr float tunnelThreshold = 0.83f;

        constexpr int step4 = 4;
        constexpr int gridSizeX4 = (Chunk::SIZE + 2 + step4 - 1) / step4 + 1;
        constexpr int gridSizeY4 = gridSizeX4;
        constexpr int gridSizeZ4 = gridSizeX4;
        const float tunnelCave = trilinearInterpolation(tunnelCavesNoises, position, gridSizeX4, gridSizeY4, gridSizeZ4,
                                                        worldX, worldY, worldZ, step4);

        isTunnel = std::abs(tunnelCave) > tunnelThreshold;
        if (!isTunnel) return false;
    }

    // Reduce cave chance near the surface
    if (worldY > columnHeight - 5) {
        const float surfaceFactor = 1.f - static_cast<float>(worldY) / (static_cast<float>(columnHeight) + 1.f);
        const float caveChance = isCheeseCave
                                     ? largeCave / 2.f * surfaceFactor
                                     : (largeCave + 1.f) / 2.f * surfaceFactor;
        return caveChance >= 0.0085f;
    }

    return true;
}

Biome TerrainGenerator::getBiome(const NoiseValues &noises, const int worldX, const int worldZ) {
    const float edgeNoise = getSurfaceFeaturesNoiseAt(worldX, worldZ) * 0.02f;

    const float continentalness = noises.continentalness + edgeNoise * 0.5f;
    const float erosion = noises.erosion + edgeNoise * 0.4f;
    const float temperature = noises.temperature + edgeNoise * 0.25f;
    const float humidity = noises.humidity + edgeNoise * 0.25f;

    // Oceans
    if (continentalness < 0.0f) {
        if (continentalness < -0.45f && erosion > 0.2f) {
            return Biome::DEEP_OCEAN;
        }
        if (erosion > 0.0f) {
            return Biome::OCEAN;
        }
        return Biome::PLAINS;
    }

    // Mountains
    if (continentalness > 0.85f) return Biome::SNOWY_MOUNTAINS;
    if (continentalness > 0.4f && erosion < 0.1f) return Biome::MOUNTAINS;

    // Biomes based on temperature and humidity
    if (temperature > 0.6f) return Biome::DESERT;
    if (temperature > 0.1f) {
        if (humidity > 0.5f)
            return Biome::JUNGLE;
        if (humidity > 0.1f)
            return Biome::PLAINS;

        return Biome::PLAINS;
    }
    if (temperature > -0.2f) {
        if (humidity > 0.1f)
            return Biome::FOREST;
        if (humidity > -0.1f)
            return Biome::PLAINS;
        return Biome::PLAINS;
    }
    if (temperature > -0.3f && humidity > -0.3f) return Biome::TAIGA;
    if (temperature > -0.7f && humidity > -0.3f) return Biome::SNOWY_TAIGA;

    return temperature <= -0.7f ? Biome::SNOWY_PLAINS : Biome::PLAINS;
}

std::string_view TerrainGenerator::getBiomeName(const Biome biome) {
    const auto idx = static_cast<size_t>(std::to_underlying(biome));
    static constexpr std::array<std::string_view, 11> names = {
        "Deep ocean", "Ocean", "Plains", "Snowy Plains", "Desert", "Forest", "Taiga", "Snowy Taiga", "Jungle",
        "Mountains", "Snowy Mountains"
    };
    constexpr size_t count = names.size();
    return idx < count ? names[idx] : std::string_view{"UNKNOWN"};
}


Block::BlockType TerrainGenerator::getBlockType(const int y, const int columnHeight, const Biome biome) {
    using enum Block::BlockType;
    const int waterLevel = getSeaLevel();

    if (y < 1) return AIR;
    if (y == 1) return BEDROCK;

    if (y <= columnHeight) {
        // Surface block
        if (y == columnHeight && columnHeight >= waterLevel) return getSurfaceBlockType(biome);
        // Disallow cave entrances underwater bc water doesn't flow into caves yet
        if (y == columnHeight) return DIRT;

        // Subsurface blocks
        if (y < columnHeight - 4) return STONE;

        // Near-surface blocks
        if (y < columnHeight && y < 200) return getNearSurfaceBlockType(biome);
    }

    if (y > columnHeight && y <= waterLevel) {
        return WATER;
    }

    return AIR;
}

bool TerrainGenerator::isSnowBiome(const Biome biome) {
    using enum Biome;
    return biome == SNOWY_MOUNTAINS || biome == SNOWY_PLAINS || biome == SNOWY_TAIGA;
}

int TerrainGenerator::getSeed() {
    return SEED;
}

FastNoiseLite TerrainGenerator::makeTerrainNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetSeed(SEED);
    noise.SetFrequency(.0055f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(6);
    noise.SetFractalLacunarity(2.2f);
    return noise;
}

FastNoiseLite TerrainGenerator::makeContinentalnessNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetSeed(SEED);
    noise.SetFrequency(.001f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(4);
    noise.SetFractalLacunarity(2.08f);
    noise.SetFractalGain(0.510f);
    noise.SetFractalWeightedStrength(0.77f);
    noise.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2Reduced);
    noise.SetDomainWarpAmp(15.f);
    return noise;
}

FastNoiseLite TerrainGenerator::makeErosionNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetSeed(SEED);
    noise.SetFrequency(.0009f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(4);
    noise.SetFractalLacunarity(2.f);
    noise.SetFractalGain(0.510f);
    noise.SetFractalWeightedStrength(1.f);
    noise.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2Reduced);
    noise.SetDomainWarpAmp(10.f);
    return noise;
}

FastNoiseLite TerrainGenerator::makeTemperatureNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noise.SetSeed(SEED);
    noise.SetFrequency(.0001f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(2);
    return noise;
}

FastNoiseLite TerrainGenerator::makeHumidityNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noise.SetSeed(SEED);
    noise.SetFrequency(.0005f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(2);
    return noise;
}


FastNoiseLite TerrainGenerator::makeSurfaceFeaturesNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetSeed(SEED);
    noise.SetFrequency(.5f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(6);
    return noise;
}

FastNoiseLite TerrainGenerator::makeLargeCaveNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetSeed(SEED);
    noise.SetFrequency(.01f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(2);
    return noise;
}

FastNoiseLite TerrainGenerator::makeTunnelCaveNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetSeed(SEED);
    noise.SetFrequency(.02f);
    noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noise.SetFractalOctaves(3);
    return noise;
}

FastNoiseLite &TerrainGenerator::getTerrainNoise() {
    thread_local FastNoiseLite instance = makeTerrainNoise();
    return instance;
}

FastNoiseLite &TerrainGenerator::getContinentalnessNoise() {
    thread_local FastNoiseLite instance = makeContinentalnessNoise();
    return instance;
}

FastNoiseLite &TerrainGenerator::getErosionNoise() {
    thread_local FastNoiseLite instance = makeErosionNoise();
    return instance;
}

FastNoiseLite &TerrainGenerator::getTemperatureNoise() {
    thread_local FastNoiseLite instance = makeTemperatureNoise();
    return instance;
}

FastNoiseLite &TerrainGenerator::getHumidityNoise() {
    thread_local FastNoiseLite instance = makeHumidityNoise();
    return instance;
}


FastNoiseLite &TerrainGenerator::getLargeCaveNoise() {
    thread_local FastNoiseLite instance = makeLargeCaveNoise();
    return instance;
}

FastNoiseLite &TerrainGenerator::getTunnelCaveNoise() {
    thread_local FastNoiseLite instance = makeTunnelCaveNoise();
    return instance;
}

float TerrainGenerator::trilinearInterpolation(const std::span<const float> &noises, const ChunkPosition &position,
                                               const int gridSizeX, const int gridSizeY, const int gridSizeZ,
                                               const int worldX, const int worldY, const int worldZ, const int step) {
    const int localX = worldX - position.x;
    const int localY = worldY - position.y;
    const int localZ = worldZ - position.z;

    const int cellX = std::clamp(localX / step, 0, gridSizeX - 2);
    const int cellY = std::clamp(localY / step, 0, gridSizeY - 2);
    const int cellZ = std::clamp(localZ / step, 0, gridSizeZ - 2);

    const float fracX = std::clamp(static_cast<float>(localX - cellX * step) / static_cast<float>(step), 0.f, 1.f);
    const float fracY = std::clamp(static_cast<float>(localY - cellY * step) / static_cast<float>(step), 0.f, 1.f);
    const float fracZ = std::clamp(static_cast<float>(localZ - cellZ * step) / static_cast<float>(step), 0.f, 1.f);

    const auto idx = [&](const int i, const int j, const int k) {
        return i + gridSizeX * (j + gridSizeY * k);
    };

    // Cube corners
    const float V000 = noises[idx(cellX, cellY, cellZ)];
    const float V100 = noises[idx(cellX + 1, cellY, cellZ)];
    const float V010 = noises[idx(cellX, cellY + 1, cellZ)];
    const float V110 = noises[idx(cellX + 1, cellY + 1, cellZ)];
    const float V001 = noises[idx(cellX, cellY, cellZ + 1)];
    const float V101 = noises[idx(cellX + 1, cellY, cellZ + 1)];
    const float V011 = noises[idx(cellX, cellY + 1, cellZ + 1)];
    const float V111 = noises[idx(cellX + 1, cellY + 1, cellZ + 1)];

    // Interpolate along x
    const float c00 = std::lerp(V000, V100, fracX);
    const float c10 = std::lerp(V010, V110, fracX);
    const float c01 = std::lerp(V001, V101, fracX);
    const float c11 = std::lerp(V011, V111, fracX);

    // Interpolate along y
    const float c0 = std::lerp(c00, c10, fracY);
    const float c1 = std::lerp(c01, c11, fracY);

    // Interpolate along z
    return std::lerp(c0, c1, fracZ);
}

FastNoiseLite &TerrainGenerator::getSurfaceFeaturesNoise() {
    thread_local FastNoiseLite instance = makeSurfaceFeaturesNoise();
    return instance;
}

int TerrainGenerator::getSeaLevel() {
    return SEA_LEVEL;
}

float TerrainGenerator::getTerrainNoiseAt(const int worldX, const int worldZ) {
    return getTerrainNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldZ));
}

float TerrainGenerator::getContinentalnessAt(const int worldX, const int worldZ) {
    return getContinentalnessNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldZ));
}

float TerrainGenerator::getErosionAt(const int worldX, const int worldZ) {
    return getErosionNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldZ));
}

float TerrainGenerator::getTemperatureAt(const int worldX, const int worldZ) {
    return getTemperatureNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldZ));
}

float TerrainGenerator::getHumidityAt(const int worldX, const int worldZ) {
    return getHumidityNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldZ));
}

float TerrainGenerator::getSurfaceFeaturesNoiseAt(const int worldX, const int worldZ) {
    return getSurfaceFeaturesNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldZ));
}

float TerrainGenerator::getLargeCaveNoiseAt(const int worldX, const int worldY, const int worldZ) {
    return getLargeCaveNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldY),
        static_cast<float>(worldZ));
}

float TerrainGenerator::getTunnelCaveNoiseAt(const int worldX, const int worldY, const int worldZ) {
    return getTunnelCaveNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldY),
        static_cast<float>(worldZ));
}

int TerrainGenerator::getBaseLevel(const NoiseValues &noises) {
    const float continentalnessLevel = getContinentalnessLevel(noises.continentalness);
    const float erosionLevel = getErosionLevel(noises.erosion);

    constexpr float continentalnessWeight = 0.8f;
    constexpr float erosionWeight = 0.2f;

    const int baseLevel = static_cast<int>(
        continentalnessLevel * continentalnessWeight +
        erosionLevel * erosionWeight
    );

    return baseLevel;
}

float TerrainGenerator::getContinentalnessLevel(const float continentalness) {
    static constexpr std::array<Step, 15> steps = {
        {
            {-1.0f, 10}, // Deep ocean
            {-0.8f, 10},
            {-0.45f, 10},
            {-0.2f, 30}, // Shallow ocean
            {-0.15f, 30},
            {0.0f, 61}, // Coastline
            {0.1f, 90}, // Lowlands
            {0.2f, 90},
            {0.3f, 115}, // Highland
            {0.4f, 130},
            {0.5f, 130},
            {0.8f, 150}, // Mountains
            {0.85f, 150},
            {0.9f, 180},
            {1.0f, 200} // High mountains
        }
    };

    for (size_t i = 1; i < std::size(steps); ++i) {
        if (continentalness <= steps[i].noise) {
            const Step a = steps[i - 1];
            const Step b = steps[i];

            // Linear interpolation
            const float t = (continentalness - a.noise) / (b.noise - a.noise);
            return std::lerp(a.height, b.height, t);
        }
    }

    return 200; // Fallback (should not happen)
}

float TerrainGenerator::getErosionLevel(const float erosion) {
    static constexpr std::array<Step, 11> steps = {
        {
            {-1.0f, 180},
            {-0.8f, 150},
            {-0.45f, 120},
            {-0.2f, 130},
            {0.0f, 50},
            {0.2f, 46},
            {0.4f, 40},
            {0.6f, 66},
            {0.7f, 66},
            {0.85f, 50},
            {1.0f, 40}
        }
    };

    for (size_t i = 1; i < std::size(steps); ++i) {
        if (erosion <= steps[i].noise) {
            const Step a = steps[i - 1];
            const Step b = steps[i];

            // Linear interpolation
            const float t = (erosion - a.noise) / (b.noise - a.noise);
            return std::lerp(a.height, b.height, t);
        }
    }

    return 180; // Fallback (should not happen)
}

Block::BlockType TerrainGenerator::getSurfaceBlockType(const Biome biome) {
    switch (biome) {
        using enum Block::BlockType;
        using enum Biome;
        case DEEP_OCEAN:
            return GRAVEL;
        case DESERT:
            return SAND;
        case SNOWY_TAIGA:
        case SNOWY_PLAINS:
            return SNOW_GRASS;
        case MOUNTAINS:
            return STONE;
        case SNOWY_MOUNTAINS:
            return SNOW;
        case JUNGLE:
            return JUNGLE_GRASS;
        default:
            return GRASS;
    }
}

Block::BlockType TerrainGenerator::getNearSurfaceBlockType(const Biome biome) {
    switch (biome) {
        using enum Block::BlockType;
        using enum Biome;
        case DESERT:
            return SAND;
        case MOUNTAINS:
            return STONE;
        case SNOWY_MOUNTAINS:
            return SNOW;
        default:
            return DIRT;
    }
}
