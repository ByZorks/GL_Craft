#ifndef FLOWERINSTANCERENDERER_H
#define FLOWERINSTANCERENDERER_H

#include "../../../render/InstanceRenderer.h"

class FlowerInstanceRenderer final : public InstanceRenderer {
private:
    void addVertices(std::vector<BlockVertex> &vertices) override;
};

#endif //FLOWERINSTANCERENDERER_H
