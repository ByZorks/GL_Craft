#include "ShortGrass.h"

ShortGrass::ShortGrass(const int x, const int y, const int z) : Vegetation(x, y, z) {
    m_blockFaceData.reserve(2);
    m_vertices.reserve(2*4);
}

void ShortGrass::generateVoxel() {
    m_blockType[0] = BlockType::SHORT_GRASS;

    m_status = Status::VOXEL_GENERATED;
}

void ShortGrass::generateMesh() {
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::SHORT_GRASS, m_vertices, 0.0f, 0.0f, 0.0f);
    m_blockFaceData.emplace_back(Face::BACK, 4);

    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::SHORT_GRASS, m_vertices, 0.0f, 0.0f, 0.0f);
    m_blockFaceData.emplace_back(Face::FRONT, 4);

    m_status = Status::MESH_GENERATED;
}

bool ShortGrass::isBillboard() const {
    return true;
}
