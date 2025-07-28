#include "Tree.h"

Tree::Tree(const int x, const int y, const int z)
    : Vegetation(x, y, z) {
    m_blockType.resize(343, BlockType::AIR);
    m_blockFaceData.reserve(6 * 83); // 6 faces, 83 blocks
    m_vertices.reserve(6 * 83 * 4); // 6 faces, 83 blocks, 4 vertices per face
}

void Tree::generateVoxel() {
    // Trunk: 1x5x1 = 5 blocks (y=0 to y=4)
    m_blockType[index(3, 0, 3)] = BlockType::LOG;
    m_blockType[index(3, 1, 3)] = BlockType::LOG;
    m_blockType[index(3, 2, 3)] = BlockType::LOG;
    m_blockType[index(3, 3, 3)] = BlockType::LOG;
    m_blockType[index(3, 4, 3)] = BlockType::LOG;

    // Leaves niveau 1: 5x2x5 = 50 blocks (y=3 to y=4)
    for (int y = 3; y < 5; y++) {
        for (int x = 1; x < 6; x++) {
            for (int z = 1; z < 6; z++) {
                if (x == 3 && y == 3 && z == 3) continue; // Skip the trunk position
                m_blockType[index(x, y, z)] = BlockType::LEAVES;
            }
        }
    }

    // Leaves niveau 2: 3x2x3 = 18 blocks (y=5 to y=6)
    for (int y = 5; y < 7; y++) {
        for (int x = 2; x < 5; x++) {
            for (int z = 2; z < 5; z++) {
                if ((x == 2 && z == 2) || (x == 4 && z == 4) || (x == 2 && z == 4) || (x == 4 && z == 2)) continue;
                // Skip corners
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
                const bool isTransparent = Block::isTransparent(blockType);
                if (shouldDrawFace(x, y + 1, z, blockType)) {
                    Block::addFaceVertices(Face::TOP, blockType, m_vertices, static_cast<float>(x),
                                           static_cast<float>(y), static_cast<float>(z));
                    m_blockFaceData.emplace_back(Face::TOP, 4, isTransparent);
                }
                if (shouldDrawFace(x, y - 1, z, blockType)) {
                    Block::addFaceVertices(Face::BOTTOM, blockType, m_vertices, static_cast<float>(x),
                                           static_cast<float>(y), static_cast<float>(z));
                    m_blockFaceData.emplace_back(Face::BOTTOM, 4, isTransparent);
                }
                if (shouldDrawFace(x, y, z + 1, blockType)) {
                    Block::addFaceVertices(Face::FRONT, blockType, m_vertices, static_cast<float>(x),
                                           static_cast<float>(y), static_cast<float>(z));
                    m_blockFaceData.emplace_back(Face::FRONT, 4, isTransparent);
                }
                if (shouldDrawFace(x, y, z - 1, blockType)) {
                    Block::addFaceVertices(Face::BACK, blockType, m_vertices, static_cast<float>(x),
                                           static_cast<float>(y), static_cast<float>(z));
                    m_blockFaceData.emplace_back(Face::BACK, 4, isTransparent);
                }
                if (shouldDrawFace(x + 1, y, z, blockType)) {
                    Block::addFaceVertices(Face::RIGHT, blockType, m_vertices, static_cast<float>(x),
                                           static_cast<float>(y), static_cast<float>(z));
                    m_blockFaceData.emplace_back(Face::RIGHT, 4, isTransparent);
                }
                if (shouldDrawFace(x - 1, y, z, blockType)) {
                    Block::addFaceVertices(Face::LEFT, blockType, m_vertices, static_cast<float>(x),
                                           static_cast<float>(y), static_cast<float>(z));
                    m_blockFaceData.emplace_back(Face::LEFT, 4, isTransparent);
                }
            }
        }
    }

    m_status = Status::MESH_GENERATED;
}

void Tree::setupBuffers() {
    std::vector<unsigned int> meshIndices_opaque;
    std::vector<unsigned int> meshIndices_transparent;
    meshIndices_opaque.reserve(m_vertices.size() * 6); // 6 indices per face
    meshIndices_transparent.reserve(m_vertices.size() * 6); // 6 indices per face
    unsigned int vertexOffset = 0;

    for (const auto &[faceType, vertexCount, isTransparent]: m_blockFaceData) {
        constexpr unsigned int faceIndicesCCW[6] = {0, 2, 1, 0, 3, 2};
        constexpr unsigned int faceIndicesCW[6] = {0, 1, 2, 0, 2, 3};
        const unsigned int *indices = faceType == Face::BACK || faceType == Face::LEFT || faceType == Face::TOP
                                          ? faceIndicesCW
                                          : faceIndicesCCW;
        for (int i = 0; i < 6; ++i) {
            unsigned int index = vertexOffset + indices[i];
            if (isTransparent) {
                meshIndices_transparent.push_back(index);
            } else {
                meshIndices_opaque.push_back(index);
            }
        }

        vertexOffset += vertexCount;
    }

    m_VBO.init(m_vertices.data(), m_vertices.size() * sizeof(BlockVertex));
    m_IBO_opaque.init(meshIndices_opaque.data(), meshIndices_opaque.size());
    m_IBO_transparent.init(meshIndices_transparent.data(), meshIndices_transparent.size());

    VertexBufferLayout chunkLayout;
    chunkLayout.Push<unsigned char>(3); // x, y, z
    chunkLayout.Push<unsigned char>(2, true); // u, v
    chunkLayout.PushInt<unsigned char>(1); // face
    m_VAO_opaque.init();
    m_VAO_opaque.AddBuffer(m_VBO, chunkLayout);
    m_VAO_transparent.init();
    m_VAO_transparent.AddBuffer(m_VBO, chunkLayout);

    m_status = Status::BUFFERS_SETUP;
}
