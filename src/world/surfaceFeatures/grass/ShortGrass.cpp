#include "ShortGrass.h"

ShortGrass::ShortGrass(const int x, const int y, const int z) : Mesh(x, y, z, SIZE) {
    m_opaqueData.faces.reserve(2);
    m_opaqueData.vertices.reserve(2*4);
}

void ShortGrass::generateVoxel() {
    m_blockType[0] = BlockType::SHORT_GRASS;

    m_state = State::VOXEL_GENERATED;
}

void ShortGrass::generateMesh() {
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::SHORT_GRASS, m_opaqueData.vertices, 0.0f, 0.0f, 0.0f);
    m_opaqueData.faces.emplace_back(Face::BACK);

    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::SHORT_GRASS, m_opaqueData.vertices, 0.0f, 0.0f, 0.0f);
    m_opaqueData.faces.emplace_back(Face::FRONT);

    m_state = State::MESH_GENERATED;
}
