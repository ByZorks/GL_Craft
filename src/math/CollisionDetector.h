#ifndef COLLISION_DETECTOR_H
#define COLLISION_DETECTOR_H

#include <unordered_map>
#include <memory>
#include "glm/glm.hpp"
#include "../world/chunk/Chunk.h"
#include "../world/chunk/ChunkPosition.h"
#include "../world/Block.h"

class CollisionDetector {
public:
    static glm::vec3 resolveCollision( const glm::vec3& currentPos, const glm::vec3& targetPos, const glm::vec3& playerSize,
        const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>>& loadedChunks);

private:
    static bool resolveAxisCollision(const glm::vec3& targetPos, int axis, const glm::vec3& halfSize,
        const glm::vec3& collisionOffset, const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>>& loadedChunks,
        glm::vec3& resolvedPos);
    static bool checkCollisionWithBlock(const AABB& playerBox, int blockX, int blockY, int blockZ,
        const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>>& loadedChunks);
    static Block::BlockType getBlockAtWorldPosition(int worldX, int worldY, int worldZ,
        const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>>& loadedChunks);
    static bool isSolidBlock(Block::BlockType blockType);
};

#endif // COLLISION_DETECTOR_H

