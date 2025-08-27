#ifndef FLOWER_H
#define FLOWER_H
#include "../../chunk/Mesh.h"

class Flower : public Mesh {
public:
    static constexpr unsigned int SIZE = 1;

    Flower(int x, int y, int z);
};

#endif //FLOWER_H
