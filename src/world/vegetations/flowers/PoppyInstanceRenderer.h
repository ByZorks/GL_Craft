#ifndef POPPYINSTANCERENDERER_H
#define POPPYINSTANCERENDERER_H

#include "../../../render/InstanceRenderer.h"

class PoppyInstanceRenderer final : public InstanceRenderer {
private:
    void addVertices(std::vector<BlockVertex> &vertices) override;
};

#endif //POPPYINSTANCERENDERER_H
