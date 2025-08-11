#include "Cornflower.h"

Cornflower::Cornflower(const int x, const int y, const int z) : Flower(x, y, z) {
}

void Cornflower::generateVoxel() {
    m_blockType[0] = BlockType::FLOWER_CORNFLOWER;

    m_state = State::VOXEL_GENERATED;
}

void Cornflower::generateMesh() {
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::FLOWER_CORNFLOWER, m_vertices, 0.0f, 0.0f, 0.0f);
    m_blockFaceData.emplace_back(Face::BACK, 4);

    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::FLOWER_CORNFLOWER, m_vertices, 0.0f, 0.0f, 0.0f);
    m_blockFaceData.emplace_back(Face::FRONT, 4);

    m_state = State::MESH_GENERATED;
}
