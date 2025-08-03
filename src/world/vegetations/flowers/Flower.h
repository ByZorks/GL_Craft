#ifndef FLOWER_H
#define FLOWER_H
#include "../Vegetation.h"

class Flower : public Vegetation {
public:
    static constexpr unsigned int SIZE = 1;

    Flower(int x, int y, int z);

    [[nodiscard]] bool isBillboard() const override;
};

#endif //FLOWER_H
