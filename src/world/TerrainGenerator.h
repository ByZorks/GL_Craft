#ifndef GL_CRAFT_TERRAINGENERATOR_H
#define GL_CRAFT_TERRAINGENERATOR_H
#include "fastNoiseLite/FastNoiseLite.h"

class TerrainGenerator {
private:
    static constexpr int SEA_LEVEL = 63;
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
private:
    static int getBaseLevel(int worldX, int worldZ);

    static FastNoiseLite makeTerrainNoise();
    static FastNoiseLite makeContinentalnessNoise();
    static FastNoiseLite makeSurfaceFeaturesNoise();
    static FastNoiseLite makeCaveNoise();

    static FastNoiseLite & getTerrainNoise();
    static FastNoiseLite & getContinentalnessNoise();
    static FastNoiseLite & getCaveNoise();
};

#endif //GL_CRAFT_TERRAINGENERATOR_H