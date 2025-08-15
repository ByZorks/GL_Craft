#ifndef GL_CRAFT_RAYCAST_H
#define GL_CRAFT_RAYCAST_H
#include <memory>
#include <unordered_map>

#include "vec3.hpp"
#include "../world/Block.h"
#include "../world/Chunk.h"
#include "../world/ChunkPosition.h"

class Raycast {
private:
    constexpr static float EPSILON = 1.f;
    constexpr static float MAX_DISTANCE = 5.f;

public:
    static BlockType castRay(const glm::vec3 &pos, const glm::vec3 &dir, const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>> &chunks);

};

#endif //GL_CRAFT_RAYCAST_H