#include "TerrainGenerator.h"

#include <algorithm>

int TerrainGenerator::getHeight(const int worldX, const int worldZ) {
    constexpr int baseHeight = 58;
    constexpr int maxHeight = 256;

    // 2D noise generation for terrain height
    const float normalizedNoise = (getTerrainNoise().GetNoise(
                                       static_cast<float>(worldX),
                                       static_cast<float>(worldZ)
                                   ) + 1.0f) / 2.0f;

    const float terrainShape = std::pow(normalizedNoise, 4.6f);
    float columnHeight = std::floor(baseHeight + terrainShape * maxHeight);

    // 3D noise generation for cave system
    const float normalized3DNoise = (getCaveNoise().GetNoise(
                                         static_cast<float>(worldX),
                                         columnHeight,
                                         static_cast<float>(worldZ)
                                     ) + 1.0f) / 2.0f;
    constexpr float baseCaveThreshold = 0.87f;
    const float surfaceModifier = 1.0f - std::clamp((columnHeight - baseHeight) / (maxHeight * 0.7f), 0.0f, 1.0f);
    const float caveThreshold = baseCaveThreshold + surfaceModifier * 0.3f;

    // Adjust column height based on cave noise
    if (std::abs(normalized3DNoise - caveThreshold) < 0.3f) {
        columnHeight -= (normalized3DNoise - (caveThreshold - 0.3f)) * 10.0f;
    }

    return static_cast<int>(columnHeight);
}

bool TerrainGenerator::isCave(const int worldX, const int worldY, const int worldZ, const int columnHeight) {
    constexpr int maxHeight = 256;
    constexpr int baseHeight = 58;
    constexpr int waterLevel = 63;

    if (worldY <= 1 || worldY > maxHeight || (worldY >= columnHeight && columnHeight < waterLevel)) return false;

    // 3D noise generation for cave system
    const float normalized3DNoise = (getCaveNoise().GetNoise(static_cast<float>(worldX), static_cast<float>(worldY),
                                                             static_cast<float>(worldZ)) + 1.0f) / 2.0f;
    // Normalize to [0, 1]
    constexpr float baseCaveThreshold = 0.87f;
    const float surfaceModifier = 1.0f - std::clamp(static_cast<float>(worldY - baseHeight) / (maxHeight * 0.7f), 0.0f,
                                                    1.0f);
    const float caveThreshold = baseCaveThreshold + surfaceModifier * 0.3f; // Increase threshold near surface

    return std::abs(normalized3DNoise - caveThreshold) < 0.3f;
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

FastNoiseLite & TerrainGenerator::getSurfaceFeaturesNoise() {
    thread_local FastNoiseLite instance = makeSurfaceFeaturesNoise();
    return instance;
}

FastNoiseLite & TerrainGenerator::getCaveNoise() {
    thread_local FastNoiseLite instance = makeCaveNoise();
    return instance;
}
