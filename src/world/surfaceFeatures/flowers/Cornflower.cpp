#include "Cornflower.h"

Cornflower::Cornflower(const int x, const int y, const int z) : Flower(x, y, z) {
}

void Cornflower::generateVoxel() {
    m_blockType[0] = BlockType::FLOWER_CORNFLOWER;

    m_state = State::VOXEL_GENERATED;
}

void Cornflower::generateMesh(const bool setFlag) {
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::FLOWER_CORNFLOWER, m_opaqueData.vertices, 0.0f, 0.0f, 0.0f);

    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::FLOWER_CORNFLOWER, m_opaqueData.vertices, 0.0f, 0.0f, 0.0f);

    if (setFlag) m_state = State::MESH_GENERATED;
}
