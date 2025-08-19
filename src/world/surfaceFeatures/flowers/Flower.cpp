#include "Flower.h"

Flower::Flower(const int x, const int y, const int z) :  Mesh(x, y, z, SIZE) {
    m_opaqueData.vertices.reserve(2);
}
