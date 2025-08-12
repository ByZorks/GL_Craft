#include "Poppy.h"

Poppy::Poppy(const int x, const int y, const int z) : Flower(x, y, z) {
}

void Poppy::generateVoxel() {
    m_blockType[0] = BlockType::FLOWER_POPPY;

    m_state = State::VOXEL_GENERATED;
}

void Poppy::generateMesh() {
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::FLOWER_POPPY, m_opaqueData.vertices, 0.0f, 0.0f, 0.0f);
    m_opaqueData.blockFaceData.emplace_back(Face::BACK, 4);

    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::FLOWER_POPPY, m_opaqueData.vertices, 0.0f, 0.0f, 0.0f);
    m_opaqueData.blockFaceData.emplace_back(Face::FRONT, 4);

    m_state = State::MESH_GENERATED;
}
