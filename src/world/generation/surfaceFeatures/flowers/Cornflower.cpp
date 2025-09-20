#include "Cornflower.h"

Cornflower::Cornflower(const int x, const int y, const int z) : Flower(x, y, z) {
}

void Cornflower::generateVoxel() {
    m_blocks[0] = Block::BlockType::FLOWER_CORNFLOWER;

    m_state = State::VOXEL_GENERATED;
}

void Cornflower::generateMesh() {
    Block::addFaceVerticesAsBilboard(Block::Face::BACK, Block::BlockType::FLOWER_CORNFLOWER, m_opaqueData.vertices,
                                     0.0f, 0.0f, 0.0f);

    Block::addFaceVerticesAsBilboard(Block::Face::FRONT, Block::BlockType::FLOWER_CORNFLOWER, m_opaqueData.vertices,
                                     0.0f, 0.0f, 0.0f);

    m_state = State::READY_TO_DRAW;
}
