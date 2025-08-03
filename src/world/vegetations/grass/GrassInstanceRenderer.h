#ifndef GRASSINSTANCERENDERER_H
#define GRASSINSTANCERENDERER_H

#include "../../../render/InstanceRenderer.h"

class GrassInstanceRenderer final : public InstanceRenderer {
public:
    void addVertices(std::vector<BlockVertex> &vertices) override;
};

#endif //GRASSINSTANCERENDERER_H
