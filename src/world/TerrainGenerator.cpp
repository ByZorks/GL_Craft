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

    const float terrainShape = std::pow(normalizedNoise, 4.f);
    float columnHeight = std::floor(static_cast<float>(baseHeight) + terrainShape * HEIGHT_MULTIPLIER);

    // 3D noise generation for cave system
    // TODO: Update cave system
    // const float normalized3DNoise = (getCaveNoise().GetNoise(
    //                                      static_cast<float>(worldX),
    //                                      columnHeight,
    //                                      static_cast<float>(worldZ)
    //                                  ) + 1.0f) / 2.0f;
    // constexpr float baseCaveThreshold = 0.87f;
    // const float surfaceModifier = 1.0f - std::clamp((columnHeight - static_cast<float>(baseHeight)) / (MAX_HEIGHT * 0.7f), 0.0f, 1.0f);
    // const float caveThreshold = baseCaveThreshold + surfaceModifier * 0.3f;
    //
    // // Adjust column height based on cave noise
    // if (std::abs(normalized3DNoise - caveThreshold) < 0.3f) {
    //     columnHeight -= (normalized3DNoise - (caveThreshold - 0.3f)) * 10.0f;
    // }

    return static_cast<int>(columnHeight);
}

bool TerrainGenerator::isCave(const int worldX, const int worldY, const int worldZ, const int columnHeight) {
    return false;

    // TODO: Update cave system
    const int baseHeight = getBaseLevel(worldX, worldZ);

    if (worldY <= 1 || worldY > HEIGHT_MULTIPLIER || (worldY >= columnHeight && columnHeight < SEA_LEVEL)) return false;

    // 3D noise generation for cave system
    const float normalized3DNoise = (getCaveNoise().GetNoise(static_cast<float>(worldX), static_cast<float>(worldY),
                                                             static_cast<float>(worldZ)) + 1.0f) / 2.0f;
    // Normalize to [0, 1]
    constexpr float baseCaveThreshold = 0.87f;
    const float surfaceModifier = 1.0f - std::clamp(static_cast<float>(worldY - baseHeight) / (HEIGHT_MULTIPLIER * 0.7f), 0.0f,
                                                    1.0f);
    const float caveThreshold = baseCaveThreshold + surfaceModifier * 0.3f; // Increase threshold near surface

    return std::abs(normalized3DNoise - caveThreshold) < 0.3f;
}

FastNoiseLite TerrainGenerator::makeTerrainNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFrequency(.0055f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(9);
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

FastNoiseLite TerrainGenerator::makeCaveNoise() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(.018f);
    noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noise.SetFractalOctaves(6);
    noise.SetFractalLacunarity(1.29f);
    noise.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2Reduced);
    noise.SetDomainWarpAmp(20.f);
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

int TerrainGenerator::getBaseLevel(const int worldX, const int worldZ) {
    const float continentalness = getContinentalnessNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldZ));

    const float erosion = getErosionNoise().GetNoise(
        static_cast<float>(worldX),
        static_cast<float>(worldZ));

    const float continentalnessLevel = getContinetalnessLevel(continentalness);
    const float erosionLevel = getErosionLevel(erosion);

    constexpr float continentalnessWeight = 0.8f;
    constexpr float erosionWeight = 0.2f;

    const int baseLevel = static_cast<int>(
        continentalnessLevel * continentalnessWeight +
        erosionLevel * erosionWeight
    );

    return baseLevel; // fallback (should not happen)
}

float TerrainGenerator::getContinetalnessLevel(const float continentalness) {
    static constexpr Step steps[] = {
        { -1.0f, 10}, // Deep ocean
        { -0.8f, 10}, //
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
        {0.85f, 150}, // Mountains
        {0.9f, 165},
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

    return 100; // Fallback (should not happen)
}


FastNoiseLite & TerrainGenerator::getCaveNoise() {
    thread_local FastNoiseLite instance = makeCaveNoise();
    return instance;
}
