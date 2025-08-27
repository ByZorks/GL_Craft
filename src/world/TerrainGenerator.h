#ifndef GL_CRAFT_TERRAINGENERATOR_H
#define GL_CRAFT_TERRAINGENERATOR_H
#include "fastNoiseLite/FastNoiseLite.h"

class TerrainGenerator {
private:
    static constexpr int SEA_LEVEL = 60;
    static constexpr int MIN_HEIGHT = 1;
    static constexpr int HEIGHT_MULTIPLIER = 256;

    struct Step {
        float noise;
        float height;
    };

public:
    static int getHeight(int worldX, int worldZ);
    static bool isCave(int worldX, int worldY, int worldZ, int columnHeight);

    static FastNoiseLite & getSurfaceFeaturesNoise();
    static int getSeaLevel();
    static float getTerrainNoiseAt(int worldX, int worldZ);
    static float getContinentalnessAt(int worldX, int worldZ);
    static float getErosionAt(int worldX, int worldZ);
private:
    static int getBaseLevel(int worldX, int worldZ);
    static float getContinetalnessLevel(float continentalness);
    static float getErosionLevel(float erosion);

    static FastNoiseLite makeTerrainNoise();
    static FastNoiseLite makeContinentalnessNoise();
    static FastNoiseLite makeErosionNoise();
    static FastNoiseLite makeSurfaceFeaturesNoise();
    static FastNoiseLite makeCaveNoise();

    static FastNoiseLite & getTerrainNoise();
    static FastNoiseLite & getContinentalnessNoise();
    static FastNoiseLite & getErosionNoise();
    static FastNoiseLite & getCaveNoise();
};

#endif //GL_CRAFT_TERRAINGENERATOR_H