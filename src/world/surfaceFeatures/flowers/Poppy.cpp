#include "Poppy.h"

Poppy::Poppy(const int x, const int y, const int z) : Flower(x, y, z) {
}

void Poppy::generateVoxel() {
    m_blocks[0] = Block::BlockType::FLOWER_POPPY;

    m_state = State::VOXEL_GENERATED;
}

void Poppy::generateMesh() {
    Block::addFaceVerticesAsBilboard(Block::Face::BACK, Block::BlockType::FLOWER_POPPY, m_opaqueData.vertices, 0.0f,
                                     0.0f, 0.0f);

    Block::addFaceVerticesAsBilboard(Block::Face::FRONT, Block::BlockType::FLOWER_POPPY, m_opaqueData.vertices, 0.0f,
                                     0.0f, 0.0f);

    m_state = State::READY_TO_DRAW;
}
