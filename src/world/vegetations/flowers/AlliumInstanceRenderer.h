#ifndef ALLIUMINSTANCERENDERER_H
#define ALLIUMINSTANCERENDERER_H
#include "../../../render/InstanceRenderer.h"

class AlliumInstanceRenderer final : public InstanceRenderer {
private:
    void addVertices(std::vector<BlockVertex> &vertices) override;
};

#endif //ALLIUMINSTANCERENDERER_H
