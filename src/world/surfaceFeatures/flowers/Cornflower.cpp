#include "Cornflower.h"

Cornflower::Cornflower(const int x, const int y, const int z) : Flower(x, y, z) {
}

void Cornflower::generateVoxel() {
    m_blockType[0] = BlockType::FLOWER_CORNFLOWER;

    m_state = State::VOXEL_GENERATED;
}

void Cornflower::generateMesh() {
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::FLOWER_CORNFLOWER, m_opaqueData.vertices, 0.0f, 0.0f, 0.0f);
    m_opaqueData.faces.emplace_back(Face::BACK);

    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::FLOWER_CORNFLOWER, m_opaqueData.vertices, 0.0f, 0.0f, 0.0f);
    m_opaqueData.faces.emplace_back(Face::FRONT);

    m_state = State::MESH_GENERATED;
}
