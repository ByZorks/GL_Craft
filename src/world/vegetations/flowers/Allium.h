#ifndef ALLIUM_H
#define ALLIUM_H
#include "Flower.h"

class Allium final : public Flower {
public:
    Allium(int x, int y, int z);

    void generateVoxel() override;
    void generateMesh() override;
};

#endif //ALLIUM_H
