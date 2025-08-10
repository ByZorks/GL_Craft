#ifndef TREE_H
#define TREE_H
#include "../../Mesh.h"

class Tree final : public Mesh {
public:
    static constexpr unsigned int SIZE = 7;

    Tree(int x, int y, int z);

    void generateVoxel() override;
    void generateMesh() override;
};

#endif //TREE_H
