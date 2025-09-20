#include "Allium.h"

Allium::Allium(const int x, const int y, const int z) : Flower(x, y, z) {
}

void Allium::generateVoxel() {
    m_blocks[0] = Block::BlockType::FLOWER_ALLIUM;

    m_state = State::VOXEL_GENERATED;
}

void Allium::generateMesh() {
    Block::addFaceVerticesAsBilboard(Block::Face::BACK, Block::BlockType::FLOWER_ALLIUM, m_opaqueData.vertices, 0.0f,
                                     0.0f, 0.0f);

    Block::addFaceVerticesAsBilboard(Block::Face::FRONT, Block::BlockType::FLOWER_ALLIUM, m_opaqueData.vertices, 0.0f,
                                     0.0f, 0.0f);

    m_state = State::READY_TO_DRAW;
}
