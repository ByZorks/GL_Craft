#ifndef INSTANCERENDERERDATA_H
#define INSTANCERENDERERDATA_H

#include <mutex>
#include <unordered_set>

#include "InstanceRenderer.h"
#include "vec3.hpp"
#include "../utils/CustomHash.h"

class InstanceRendererData {
public:
    std::unordered_set<glm::vec3> instances;
    mutable std::mutex mutex;
    InstanceRenderer renderer;
};

#endif //INSTANCERENDERERDATA_H
