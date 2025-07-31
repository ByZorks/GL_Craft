#ifndef VEGETATION_H
#define VEGETATION_H
#include <vector>

#include "../Mesh.h"

enum class BlockType : uint8_t;

class Vegetation : public Mesh {

public:
    static constexpr unsigned int SIZE = 16;

    Vegetation(const int x, const int y, const int z) : Mesh(x, y, z) {
        m_blockType.resize(SIZE * SIZE * SIZE, BlockType::AIR);
    }
};

#endif //VEGETATION_H
