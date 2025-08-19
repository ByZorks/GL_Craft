#include "Poppy.h"

Poppy::Poppy(const int x, const int y, const int z) : Flower(x, y, z) {
}

void Poppy::generateVoxel() {
    m_blockType[0] = BlockType::FLOWER_POPPY;

    m_state = State::VOXEL_GENERATED;
}

void Poppy::generateMesh(const bool setFlag) {
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::FLOWER_POPPY, m_opaqueData.vertices, 0.0f, 0.0f, 0.0f);

    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::FLOWER_POPPY, m_opaqueData.vertices, 0.0f, 0.0f, 0.0f);

    if (setFlag) m_state = State::MESH_GENERATED;
}
