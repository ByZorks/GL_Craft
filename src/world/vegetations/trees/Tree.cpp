#include "Tree.h"

Tree::Tree(const int x, const int y, const int z)
    : Mesh(x - 4, y, z - 4, SIZE) {
    m_blockFaceData.reserve(22);
    m_blockFaceData_transparent.reserve(348);
    m_vertices.reserve(88);
    m_vertices_transparent.reserve(1392);
}

void Tree::generateVoxel() {
    // Trunk: 1x5x1 = 5 blocks (y=0 to y=4)
    m_blockType[index(3, 0, 3)] = BlockType::LOG;
    m_blockType[index(3, 1, 3)] = BlockType::LOG;
    m_blockType[index(3, 2, 3)] = BlockType::LOG;
    m_blockType[index(3, 3, 3)] = BlockType::LOG;
    m_blockType[index(3, 4, 3)] = BlockType::LOG;

    // Leaves: 5x2x5 = 50 blocks (y=3 to y=4)
    for (int y = 3; y < 5; y++) {
        for (int x = 1; x < 6; x++) {
            for (int z = 1; z < 6; z++) {
                if (x == 3 && z == 3) continue; // Skip the trunk position
                m_blockType[index(x, y, z)] = BlockType::LEAVES;
            }
        }
    }

    // Leaves: 3x2x3 = 18 blocks (y=5 to y=6)
    for (int y = 5; y < 7; y++) {
        for (int x = 2; x < 5; x++) {
            for (int z = 2; z < 5; z++) {
                // Skip corners
                if ((x == 2 && z == 2) || (x == 4 && z == 4) || (x == 2 && z == 4) || (x == 4 && z == 2)) continue;
                m_blockType[index(x, y, z)] = BlockType::LEAVES;
            }
        }
    }

    m_status = Status::VOXEL_GENERATED;
}

void Tree::generateMesh() {
    for (int x = 0; x < SIZE; ++x) {
        for (int y = 0; y < SIZE; ++y) {
            for (int z = 0; z < SIZE; ++z) {
                const BlockType blockType = m_blockType[index(x, y, z)];
                if (blockType == BlockType::AIR) continue;
                const auto localXf = static_cast<float>(x);
                const auto localYf = static_cast<float>(y);
                const auto localZf = static_cast<float>(z);
                const bool isTransparent = Block::isTransparent(blockType);

                if (shouldDrawFace(x, y + 1, z, blockType)) {
                    if (isTransparent) {
                        Block::addFaceVertices(Face::TOP, blockType, m_vertices_transparent, localXf, localYf, localZf);
                        m_blockFaceData_transparent.emplace_back(Face::TOP, 4);
                    } else {
                        Block::addFaceVertices(Face::TOP, blockType, m_vertices, localXf, localYf, localZf);
                        m_blockFaceData.emplace_back(Face::TOP, 4);
                    }
                }
                if (shouldDrawFace(x, y - 1, z, blockType)) {
                    if (isTransparent) {
                        Block::addFaceVertices(Face::BOTTOM, blockType, m_vertices_transparent, localXf, localYf, localZf);
                        m_blockFaceData_transparent.emplace_back(Face::BOTTOM, 4);
                    } else {
                        Block::addFaceVertices(Face::BOTTOM, blockType, m_vertices, localXf, localYf, localZf);
                        m_blockFaceData.emplace_back(Face::BOTTOM, 4);
                    }
                }
                if (shouldDrawFace(x, y, z + 1, blockType)) {
                    if (isTransparent) {
                        Block::addFaceVertices(Face::FRONT, blockType, m_vertices_transparent, localXf, localYf, localZf);
                        m_blockFaceData_transparent.emplace_back(Face::FRONT, 4);
                    } else {
                        Block::addFaceVertices(Face::FRONT, blockType, m_vertices, localXf, localYf, localZf);
                        m_blockFaceData.emplace_back(Face::FRONT, 4);
                    }
                }
                if (shouldDrawFace(x, y, z - 1, blockType)) {
                    if (isTransparent) {
                        Block::addFaceVertices(Face::BACK, blockType, m_vertices_transparent, localXf, localYf, localZf);
                        m_blockFaceData_transparent.emplace_back(Face::BACK, 4);
                    } else {
                        Block::addFaceVertices(Face::BACK, blockType, m_vertices, localXf, localYf, localZf);
                        m_blockFaceData.emplace_back(Face::BACK, 4);
                    }
                }
                if (shouldDrawFace(x + 1, y, z, blockType)) {
                    if (isTransparent) {
                        Block::addFaceVertices(Face::RIGHT, blockType, m_vertices_transparent, localXf, localYf, localZf);
                        m_blockFaceData_transparent.emplace_back(Face::RIGHT, 4);
                    } else {
                        Block::addFaceVertices(Face::RIGHT, blockType, m_vertices, localXf, localYf, localZf);
                        m_blockFaceData.emplace_back(Face::RIGHT, 4);
                    }
                }
                if (shouldDrawFace(x - 1, y, z, blockType)) {
                    if (isTransparent) {
                        Block::addFaceVertices(Face::LEFT, blockType, m_vertices_transparent, localXf, localYf, localZf);
                        m_blockFaceData_transparent.emplace_back(Face::LEFT, 4);
                    } else {
                        Block::addFaceVertices(Face::LEFT, blockType, m_vertices, localXf, localYf, localZf);
                        m_blockFaceData.emplace_back(Face::LEFT, 4);
                    }
                }
            }
        }
    }

    m_status = Status::MESH_GENERATED;
}
