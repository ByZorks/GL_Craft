#ifndef GL_CRAFT_RAYCAST_H
#define GL_CRAFT_RAYCAST_H
#include <memory>
#include <unordered_map>

#include "../world/Block.h"
#include "../world/Chunk.h"
#include "../world/ChunkPosition.h"
#include "glm/vec3.hpp"

struct RaycastResult {
    std::shared_ptr<Chunk> chunk;
    std::array<int, 3> blockLocalPosition;
    glm::vec3 blockWorldPosition;
    bool hitBlock;
    BlockType blockType;
    glm::ivec3 normal;
};

class Raycast {
private:
    static int m_lastChunk0X, m_lastChunk0Y, m_lastChunk0Z;
    static int m_lastChunk1X, m_lastChunk1Y, m_lastChunk1Z;
    static int m_lastChunk2X, m_lastChunk2Y, m_lastChunk2Z;
    static std::array<std::shared_ptr<Chunk>, 3> m_cachedChunks; // In the worst case, we can hit 3 chunks in a row

public:
    static RaycastResult castRay(const glm::vec3 &rayStart, const glm::vec3 &rayDir, const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>> &chunks);
    static void clearCache();

};

#endif //GL_CRAFT_RAYCAST_H