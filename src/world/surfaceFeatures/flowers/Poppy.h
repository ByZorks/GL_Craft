#ifndef POPPY_H
#define POPPY_H
#include "Flower.h"

class Poppy final : public Flower {
public:
    Poppy(int x, int y, int z);

    void generateVoxel() override;
    void generateMesh(bool setFlag, MeshsingResult &result) override;
};

#endif //POPPY_H
