#include "AlliumInstanceRenderer.h"

void AlliumInstanceRenderer::addVertices(std::vector<BlockVertex> &vertices) {
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::FLOWER_ALLIUM, vertices, 0.0f, 0.0f, 0.0f);
    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::FLOWER_ALLIUM, vertices, 0.0f, 0.0f, 0.0f);
}
