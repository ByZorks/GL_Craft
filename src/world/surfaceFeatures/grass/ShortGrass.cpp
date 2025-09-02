#include "ShortGrass.h"

ShortGrass::ShortGrass(const int x, const int y, const int z) : Mesh(x, y, z, SIZE) {
    m_opaqueData.vertices.reserve(2);
}

void ShortGrass::generateVoxel() {
    m_blocks[0] = BlockType::SHORT_GRASS;

    m_state = State::VOXEL_GENERATED;
}

void ShortGrass::generateMesh() {
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::SHORT_GRASS, m_opaqueData.vertices, 0.0f, 0.0f, 0.0f);

    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::SHORT_GRASS, m_opaqueData.vertices, 0.0f, 0.0f, 0.0f);

    m_state = State::READY_TO_DRAW;
}
