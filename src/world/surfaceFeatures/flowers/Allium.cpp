#include "Allium.h"

Allium::Allium(const int x, const int y, const int z) : Flower(x, y, z) {
}

void Allium::generateVoxel() {
    m_blockType[0] = BlockType::FLOWER_ALLIUM;

    m_state = State::VOXEL_GENERATED;
}

void Allium::generateMesh() {
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::FLOWER_ALLIUM, m_vertices, 0.0f, 0.0f, 0.0f);
    m_blockFaceData.emplace_back(Face::BACK, 4);

    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::FLOWER_ALLIUM, m_vertices, 0.0f, 0.0f, 0.0f);
    m_blockFaceData.emplace_back(Face::FRONT, 4);

    m_state = State::MESH_GENERATED;
}
