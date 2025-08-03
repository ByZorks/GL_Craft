#include "CornflowerInstanceRenderer.h"

void CornflowerInstanceRenderer::addVertices(std::vector<BlockVertex> &vertices) {
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::FLOWER_CORNFLOWER, vertices, 0.0f, 0.0f, 0.0f);
    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::FLOWER_CORNFLOWER, vertices, 0.0f, 0.0f, 0.0f);
}
