#include "TerrainGenerator.h"

#include <algorithm>
#include <iterator>

int TerrainGenerator::getHeight(const int worldX, const int worldZ) {
    const int baseHeight = getBaseLevel(worldX, worldZ);

    // 2D noise generation for terrain height
    const float normalizedNoise = (getTerrainNoise().GetNoise(
                                       static_cast<float>(worldX),
                                       static_cast<float>(worldZ)
                                   ) + 1.0f) / 2.0f;

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

FastNoiseLite TerrainGenerator::makeTerrainNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFrequency(.0055f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(6);
    noise.SetFractalLacunarity(2.2f);
    return noise;
}

FastNoiseLite TerrainGenerator::makeContinentalnessNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
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

FastNoiseLite TerrainGenerator::makeSurfaceFeaturesNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(.5f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(6);
    return noise;
}

FastNoiseLite TerrainGenerator::makeLargeCaveNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(.01f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(2);
    return noise;
}

FastNoiseLite TerrainGenerator::makeTunnelCaveNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
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

float TerrainGenerator::getSurfaceFeaturesNoiseAt(const int worldX, const int worldZ) {
    return getSurfaceFeaturesNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldZ));
}

int TerrainGenerator::getBaseLevel(const int worldX, const int worldZ) {
    const float continentalness = getContinentalnessNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldZ));

    const float erosion = getErosionNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldZ));

    const float continentalnessLevel = getContinentalnessLevel(continentalness);
    const float erosionLevel = getErosionLevel(erosion);

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
