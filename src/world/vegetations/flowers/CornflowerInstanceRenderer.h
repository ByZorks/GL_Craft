#ifndef CORNFLOWERINSTANCERENDERER_H
#define CORNFLOWERINSTANCERENDERER_H
#include <vector>

#include "../../../render/InstanceRenderer.h"

class CornflowerInstanceRenderer final : public InstanceRenderer {
private:
    void addVertices(std::vector<BlockVertex> &vertices);
};

#endif //CORNFLOWERINSTANCERENDERER_H
