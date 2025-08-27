#ifndef GL_CRAFT_TERRAINGENERATOR_H
#define GL_CRAFT_TERRAINGENERATOR_H
#include "fastNoiseLite/FastNoiseLite.h"

class TerrainGenerator {
public:
    static int getHeight(int worldX, int worldZ);
    static bool isCave(int worldX, int worldY, int worldZ, int columnHeight);

    static FastNoiseLite & getSurfaceFeaturesNoise();
private:
    static FastNoiseLite makeTerrainNoise();
    static FastNoiseLite makeSurfaceFeaturesNoise();
    static FastNoiseLite makeCaveNoise();

    static FastNoiseLite & getTerrainNoise();
    static FastNoiseLite & getCaveNoise();
};

#endif //GL_CRAFT_TERRAINGENERATOR_H