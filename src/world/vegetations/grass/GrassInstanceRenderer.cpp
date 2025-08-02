#include "GrassInstanceRenderer.h"

void GrassInstanceRenderer::addVertices(std::vector<BlockVertex> &vertices) {
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::SHORT_GRASS, vertices, 0.0f, 0.0f, 0.0f);
    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::SHORT_GRASS, vertices, 0.0f, 0.0f, 0.0f);
}
