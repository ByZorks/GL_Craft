#ifndef GL_CRAFT_RAYCAST_H
#define GL_CRAFT_RAYCAST_H
#include <memory>
#include <unordered_map>

#include "../world/Block.h"
#include "../world/chunk/Chunk.h"
#include "../world/chunk/ChunkPosition.h"
#include "glm/vec3.hpp"

struct RaycastResult {
    std::shared_ptr<Chunk> chunk;
    std::array<int, 3> blockLocalPosition = {0, 0, 0};
    glm::vec3 blockWorldPosition = {0.f, 0.f, 0.f};
    glm::ivec3 normal = {0, 0, 0};
    bool hasHitBlock = false;
    Block::BlockType blockType = Block::BlockType::AIR;
};

class Raycast {
public:
    static RaycastResult castRay(const glm::vec3 &rayStart, const glm::vec3 &rayDir,
                                 const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk> > &chunks);

private:
    static glm::ivec3 calculateChunkCoordinates(const glm::vec3 &rayCurrentPos);
    static int findOrCacheChunk(const glm::ivec3 &chunkCoords,
                                const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk> > &chunks);
    static bool checkBlockCollision(int chunkIndex, const glm::ivec3 &localCoords, Block::BlockType &outBlockType);
    static void updateRayStep(glm::vec3 &rayCurrentPos, float &distance, glm::vec3 &rayLength1D,
                              const glm::vec3 &step, const glm::vec3 &rayUnitStepSize, glm::ivec3 &hitNormal);

private:
    static int m_lastChunk0X, m_lastChunk0Y, m_lastChunk0Z;
    static int m_lastChunk1X, m_lastChunk1Y, m_lastChunk1Z;
    static int m_lastChunk2X, m_lastChunk2Y, m_lastChunk2Z;
    static std::array<std::shared_ptr<Chunk>, 3> m_cachedChunks; // In the worst case, we can hit 3 chunks in a row
};

#endif //GL_CRAFT_RAYCAST_H
