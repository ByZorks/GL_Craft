#ifndef SHORTGRASS_H
#define SHORTGRASS_H
#include "../../Mesh.h"

class ShortGrass final : public Mesh {
public:
    static constexpr unsigned int SIZE = 1;

    ShortGrass(int x, int y, int z);

    void generateVoxel() override;
    void generateMesh() override;
};

#endif //SHORTGRASS_H
