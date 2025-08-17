#ifndef CORNFLOWER_H
#define CORNFLOWER_H
#include "Flower.h"

class Cornflower final : public Flower {
public:
    Cornflower(int x, int y, int z);

    void generateVoxel() override;
    void generateMesh(bool setFlag) override;
};

#endif //CORNFLOWER_H
