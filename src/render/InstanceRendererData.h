#ifndef INSTANCERENDERERDATA_H
#define INSTANCERENDERERDATA_H

#include <mutex>
#include <unordered_set>

#include "vec3.hpp"
#include "../utils/CustomHash.h"

template<typename RendererType>
class InstanceRendererData {
public:
    std::unordered_set<glm::vec3> instances;
    mutable std::mutex mutex;
    RendererType renderer;

    InstanceRendererData() = default;
};

#endif //INSTANCERENDERERDATA_H
