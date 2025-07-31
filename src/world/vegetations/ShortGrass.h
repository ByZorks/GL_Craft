#ifndef SHORTGRASS_H
#define SHORTGRASS_H
#include "Vegetation.h"

class ShortGrass final : public Vegetation {
public:
    ShortGrass(int x, int y, int z);

    void generateVoxel() override;
    void generateMesh() override;

    [[nodiscard]] bool isBillboard() const override;
};

#endif //SHORTGRASS_H
