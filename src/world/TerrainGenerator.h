#ifndef GL_CRAFT_TERRAINGENERATOR_H
#define GL_CRAFT_TERRAINGENERATOR_H
#include <cstdint>
#include <span>

#include "Block.h"
#include "chunk/ChunkPosition.h"
#include "fastNoiseLite/FastNoiseLite.h"

enum class Biome : uint8_t {
    DEEP_OCEAN, OCEAN, PLAINS, SNOWY_PLAINS, DESERT, FOREST, TAIGA, SNOWY_TAIGA, JUNGLE, MOUNTAINS, SNOWY_MOUNTAINS,
};

class TerrainGenerator {
private:
    static constexpr int SEED = 1337;
    static constexpr int SEA_LEVEL = 60;
    static constexpr int MIN_HEIGHT = 1;
    static constexpr int HEIGHT_MULTIPLIER = 256;

    struct Step {
        float noise;
        float height;
    };

public:
    struct NoiseValues {
        float terrain = 0.0f;
        float continentalness = 0.0f;
        float erosion = 0.0f;
        float temperature = 0.0f;
        float humidity = 0.0f;
        float surfaceFeatures = 0.0f;
        // 3D noise is not included here because it's not needed for height and biome determination, and computation can be avoided sometimes

        NoiseValues() = default;

        NoiseValues(const int worldX, const int worldZ)
            : terrain(getTerrainNoiseAt(worldX, worldZ)),
              continentalness(getContinentalnessAt(worldX, worldZ)),
              erosion(getErosionAt(worldX, worldZ)),
              temperature(getTemperatureAt(worldX, worldZ)),
              humidity(getHumidityAt(worldX, worldZ)),
              surfaceFeatures(getSurfaceFeaturesNoiseAt(worldX, worldZ)) {
        }

        void computeRemainingNoises(const int worldX, const int worldZ) {
            terrain != 0.0f ? terrain : terrain = getTerrainNoiseAt(worldX, worldZ);
            continentalness != 0.0f ? continentalness : continentalness = getContinentalnessAt(worldX, worldZ);
            erosion != 0.0f ? erosion : erosion = getErosionAt(worldX, worldZ);
            temperature != 0.0f ? temperature : temperature = getTemperatureAt(worldX, worldZ);
            humidity != 0.0f ? humidity : humidity = getHumidityAt(worldX, worldZ);
            surfaceFeatures != 0.0f ? surfaceFeatures : surfaceFeatures = getSurfaceFeaturesNoiseAt(worldX, worldZ);
        }

        void computeHeightNoises(const int worldX, const int worldZ) {
            terrain = getTerrainNoiseAt(worldX, worldZ);
            continentalness = getContinentalnessAt(worldX, worldZ);
            erosion = getErosionAt(worldX, worldZ);
        }

    };

    static int getHeight(const NoiseValues &noises);
    static bool isCave(const ChunkPosition &position, int worldX, int worldY, int worldZ, int columnHeight, const std::span<const float> &largeCavesNoises, const std::span<const float> &tunnelCavesNoises);
    static Biome getBiome(const NoiseValues &noises);
    static const char *getBiomeName(Biome biome);
    static BlockType getBlockType(int y, int columnHeight, Biome biome);
    static BlockType getNearSurfaceBlockType(Biome biome);

    static bool isSnowBiome(Biome biome);
    static int getSeed();
    static int getSeaLevel();
    static float getTerrainNoiseAt(int worldX, int worldZ);
    static float getContinentalnessAt(int worldX, int worldZ);
    static float getErosionAt(int worldX, int worldZ);
    static float getTemperatureAt(int worldX, int worldZ);
    static float getHumidityAt(int worldX, int worldZ);
    static float getSurfaceFeaturesNoiseAt(int worldX, int worldZ);
    static float getLargeCaveNoiseAt(int worldX, int worldY, int worldZ);
    static float getTunnelCaveNoiseAt(int worldX, int worldY, int worldZ);

private:
    static int getBaseLevel(const NoiseValues &noises);
    static float getContinentalnessLevel(float continentalness);
    static float getErosionLevel(float erosion);

    static BlockType getSurfaceBlockType(Biome biome);

    static FastNoiseLite makeTerrainNoise();
    static FastNoiseLite makeContinentalnessNoise();
    static FastNoiseLite makeErosionNoise();
    static FastNoiseLite makeTemperatureNoise();
    static FastNoiseLite makeHumidityNoise();
    static FastNoiseLite makeSurfaceFeaturesNoise();
    static FastNoiseLite makeLargeCaveNoise();
    static FastNoiseLite makeTunnelCaveNoise();

    static FastNoiseLite & getTerrainNoise();
    static FastNoiseLite & getContinentalnessNoise();
    static FastNoiseLite & getErosionNoise();
    static FastNoiseLite & getTemperatureNoise();
    static FastNoiseLite & getHumidityNoise();
    static FastNoiseLite & getSurfaceFeaturesNoise();
    static FastNoiseLite & getLargeCaveNoise();
    static FastNoiseLite & getTunnelCaveNoise();

    static float trilinearInterpolation(const std::span<const float> &noises, const ChunkPosition &position, int gridSizeX, int gridSizeY, int gridSizeZ, int worldX, int worldY, int worldZ, int step);
};

#endif //GL_CRAFT_TERRAINGENERATOR_H