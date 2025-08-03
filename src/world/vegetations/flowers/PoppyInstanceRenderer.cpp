#include "PoppyInstanceRenderer.h"

#include "../../Block.h"

void PoppyInstanceRenderer::addVertices(std::vector<BlockVertex> &vertices) {
    Block::addFaceVerticesAsBilboard(Face::BACK, BlockType::FLOWER_POPPY, vertices, 0.0f, 0.0f, 0.0f);
    Block::addFaceVerticesAsBilboard(Face::FRONT, BlockType::FLOWER_POPPY, vertices, 0.0f, 0.0f, 0.0f);
}
