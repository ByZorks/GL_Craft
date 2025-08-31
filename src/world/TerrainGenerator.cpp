#include "TerrainGenerator.h"

#include <algorithm>
#include <iterator>

int TerrainGenerator::getHeight(const NoiseValues &noises) {
    const int baseHeight = getBaseLevel(noises);

    // 2D noise generation for terrain height
    const float normalizedNoise = (noises.terrain + 1.0f) / 2.0f;

    const float terrainShape = normalizedNoise * normalizedNoise * normalizedNoise * normalizedNoise;
    const float columnHeight = std::floor(static_cast<float>(baseHeight) + terrainShape * HEIGHT_MULTIPLIER);

    return static_cast<int>(columnHeight);
}

bool TerrainGenerator::isCave(const int worldX, const int worldY, const int worldZ, const int columnHeight) {
    if (worldY <= 1 || worldY > HEIGHT_MULTIPLIER || (worldY >= columnHeight && columnHeight < SEA_LEVEL)) return false;

    const float largeCave = getLargeCaveNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldY),
        static_cast<float>(worldZ));

    constexpr float cheeseThreshold = 0.6f;
    const bool isCheeseCave = largeCave > cheeseThreshold;

    // Tunnel caves only if not already a cheese cave
    if (!isCheeseCave) {
        bool isTunnel = false;
        constexpr float tunnelThreshold = 0.83f;

        const float tunnelCave = getTunnelCaveNoise().GetNoise(
            static_cast<float>(worldX),
            static_cast<float>(worldY),
            static_cast<float>(worldZ));

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

Biome TerrainGenerator::getBiome(const NoiseValues &noises) {
    if (noises.continentalness < -0.45f && noises.erosion > 0.2f) return Biome::DEEP_OCEAN;
    if (noises.continentalness < 0.0f && noises.erosion > 0.0f) return Biome::OCEAN;
    if (noises.continentalness > 0.4f && noises.continentalness < 0.85f && noises.erosion < 0.1f) return Biome::MOUTAINS;
    if (noises.continentalness > 0.85f) return Biome::SNOWY_MOUTAINS;

    if (noises.temperature > 0.6f) return Biome::DESERT;
    if (noises.temperature > 0.1f && noises.humidity > 0.5f) return Biome::JUNGLE;
    if (noises.temperature > 0.1f && noises.humidity > 0.1f) return Biome::PLAINS;
    if (noises.temperature > -0.2f && noises.humidity > 0.1f) return Biome::FOREST;
    if (noises.temperature > -0.2f && noises.humidity > -0.1f) return Biome::PLAINS;
    if (noises.temperature > -0.3f && noises.humidity > -0.3f) return Biome::TAIGA;
    if (noises.temperature > -0.6f && noises.humidity > -0.3f) return Biome::SNOWY_TAIGA;
    if (noises.temperature <= -0.7f) return Biome::SNOWY_PLAINS;
    return Biome::PLAINS;
}

const char * TerrainGenerator::getBiomeName(const Biome biome) {
    switch (biome) {
        case Biome::DEEP_OCEAN: return "Deep Ocean";
        case Biome::OCEAN: return "Ocean";
        case Biome::PLAINS: return "Plains";
        case Biome::SNOWY_PLAINS: return "Snowy Plains";
        case Biome::DESERT: return "Desert";
        case Biome::FOREST: return "Forest";
        case Biome::TAIGA: return "Taiga";
        case Biome::SNOWY_TAIGA: return "Snowy Taiga";
        case Biome::JUNGLE: return "Jungle";
        case Biome::MOUTAINS: return "Moutains";
        case Biome::SNOWY_MOUTAINS: return "Snowy Moutains";
        default: return "Unknown";
    }
}


BlockType TerrainGenerator::getBlockType(const int y, const int columnHeight, const Biome biome) {
    const int waterLevel = getSeaLevel();

    if (y < 1) return BlockType::AIR;
    if (y == 1) return BlockType::BEDROCK;

    if (y <= columnHeight) {
        // Surface block
        if (y == columnHeight && columnHeight >= waterLevel) return getSurfaceBlockType(biome);
        if (y == columnHeight) return BlockType::DIRT; // Disallow cave entrances underwater bc water doesn't flow into caves yet

        // Subsurface blocks
        if (y < columnHeight - 4) return BlockType::STONE;

        // Near-surface blocks
        if (y < columnHeight && y < 200) return getNearSurfaceBlockType(biome);
    }

    if (y > columnHeight && y <= waterLevel) {
        return BlockType::WATER;
    }

    return BlockType::AIR;
}

bool TerrainGenerator::isSnowBiome(const Biome biome) {
    return biome == Biome::SNOWY_MOUTAINS || biome == Biome::SNOWY_PLAINS || biome == Biome::SNOWY_TAIGA;
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
    noise.SetFrequency(.0005f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(2);
    return noise;
}

FastNoiseLite TerrainGenerator::makeHumidityNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noise.SetSeed(SEED);
    noise.SetFrequency(.001f);
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

FastNoiseLite & TerrainGenerator::getTerrainNoise() {
    thread_local FastNoiseLite instance = makeTerrainNoise();
    return instance;
}

FastNoiseLite & TerrainGenerator::getContinentalnessNoise() {
    thread_local FastNoiseLite instance = makeContinentalnessNoise();
    return instance;
}

FastNoiseLite & TerrainGenerator::getErosionNoise() {
    thread_local FastNoiseLite instance = makeErosionNoise();
    return instance;
}

FastNoiseLite & TerrainGenerator::getTemperatureNoise() {
    thread_local FastNoiseLite instance = makeTemperatureNoise();
    return instance;
}

FastNoiseLite & TerrainGenerator::getHumidityNoise() {
    thread_local FastNoiseLite instance = makeHumidityNoise();
    return instance;
}


FastNoiseLite & TerrainGenerator::getLargeCaveNoise() {
    thread_local FastNoiseLite instance = makeLargeCaveNoise();
    return instance;
}

FastNoiseLite & TerrainGenerator::getTunnelCaveNoise() {
    thread_local FastNoiseLite instance = makeTunnelCaveNoise();
    return instance;
}

FastNoiseLite & TerrainGenerator::getSurfaceFeaturesNoise() {
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
    static constexpr Step steps[] = {
        { -1.0f, 10}, // Deep ocean
        { -0.8f, 10},
        { -0.45f, 10},
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
    };

    for (size_t i = 1; i < std::size(steps); ++i) {
        if (continentalness <= steps[i].noise) {
            const Step &a = steps[i - 1];
            const Step &b = steps[i];

            // Linear interpolation
            const float t = (continentalness - a.noise) / (b.noise - a.noise);
            return std::lerp(a.height, b.height, t);
        }
    }

    return 200; // Fallback (should not happen)
}

float TerrainGenerator::getErosionLevel(const float erosion) {
    static constexpr Step steps[] = {
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
    };

    for (size_t i = 1; i < std::size(steps); ++i) {
        if (erosion <= steps[i].noise) {
            const Step &a = steps[i - 1];
            const Step &b = steps[i];

            // Linear interpolation
            const float t = (erosion - a.noise) / (b.noise - a.noise);
            return std::lerp(a.height, b.height, t);
        }
    }

    return 180; // Fallback (should not happen)
}

BlockType TerrainGenerator::getSurfaceBlockType(const Biome biome) {
    switch (biome) {
        case Biome::DEEP_OCEAN:
            return BlockType::GRAVEL;
        case Biome::DESERT:
            return BlockType::SAND;
        case Biome::SNOWY_TAIGA: case Biome::SNOWY_PLAINS:
            return BlockType::SNOW_GRASS;
        case Biome::MOUTAINS:
            return BlockType::STONE;
        case Biome::SNOWY_MOUTAINS:
            return BlockType::SNOW;
        case Biome::JUNGLE:
            return BlockType::JUNGLE_GRASS;
        default:
            return BlockType::GRASS;
    }
}

BlockType TerrainGenerator::getNearSurfaceBlockType(const Biome biome) {
    switch (biome) {
        case Biome::DESERT:
            return BlockType::SAND;
        case Biome::MOUTAINS:
            return BlockType::STONE;
        case Biome::SNOWY_MOUTAINS:
            return BlockType::SNOW;
        default:
            return BlockType::DIRT;
    }
}
