#ifndef TREE_H
#define TREE_H
#include "../Vegetation.h"

class Tree final : public Vegetation {
public:
    static constexpr unsigned int SIZE = 7;

    Tree(int x, int y, int z);

    void generateVoxel() override;
    void generateMesh() override;
};

#endif //TREE_H
