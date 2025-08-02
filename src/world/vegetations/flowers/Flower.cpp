#include "Flower.h"

Flower::Flower(const int x, const int y, const int z) :  Vegetation(x, y, z) {
    m_blockType.resize(1, BlockType::AIR);
    m_blockFaceData.reserve(2);
    m_vertices.reserve(2*4);
}

bool Flower::isBillboard() const {
    return true;
}
