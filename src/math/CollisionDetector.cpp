#include "CollisionDetector.h"
#include <cmath>
#include <algorithm>

glm::vec3 CollisionDetector::resolveCollision(const glm::vec3 &currentPos, const glm::vec3 &targetPos,
    const glm::vec3 &playerSize, const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk> > &loadedChunks) {
    glm::vec3 resolvedPos = currentPos;
    const glm::vec3 halfSize = playerSize * 0.5f;

    const float eyeHeight = playerSize.y * 0.9f;
    const glm::vec3 collisionOffset(0.0f, -eyeHeight + halfSize.y, 0.0f);

    for (int axis = 0; axis < 3; ++axis) {
        resolveAxisCollision(targetPos, axis, halfSize, collisionOffset, loadedChunks, resolvedPos);
    }

    return resolvedPos;
}

bool CollisionDetector::resolveAxisCollision(const glm::vec3& targetPos, const int axis, const glm::vec3& halfSize,
    const glm::vec3& collisionOffset, const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>>& loadedChunks,
    glm::vec3& resolvedPos) {

    glm::vec3 testPos = resolvedPos;
    testPos[axis] = targetPos[axis];

    const AABB testBox(testPos + collisionOffset, halfSize);

    const auto minX = static_cast<int>(std::floor(testBox.getMin().x));
    const auto maxX = static_cast<int>(std::floor(testBox.getMax().x));
    const auto minY = static_cast<int>(std::floor(testBox.getMin().y));
    const auto maxY = static_cast<int>(std::floor(testBox.getMax().y));
    const auto minZ = static_cast<int>(std::floor(testBox.getMin().z));
    const auto maxZ = static_cast<int>(std::floor(testBox.getMax().z));

    bool collisionDetected = false;
    for (int x = minX; x <= maxX && !collisionDetected; ++x) {
        for (int y = minY; y <= maxY && !collisionDetected; ++y) {
            for (int z = minZ; z <= maxZ && !collisionDetected; ++z) {
                if (checkCollisionWithBlock(testBox, x, y, z, loadedChunks)) {
                    collisionDetected = true;
                }
            }
        }
    }

    if (!collisionDetected) {
        resolvedPos[axis] = targetPos[axis];
    }

    return collisionDetected;
}

bool CollisionDetector::checkCollisionWithBlock(const AABB &playerBox, const int blockX, const int blockY, const int blockZ,
                                                const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk> > &loadedChunks) {
    if (const Block::BlockType blockType = getBlockAtWorldPosition(blockX, blockY, blockZ, loadedChunks);
        !isSolidBlock(blockType)) {
        return false;
    }

    const AABB blockBox(
        glm::vec3(static_cast<float>(blockX) + 0.5f, static_cast<float>(blockY) + 0.5f, static_cast<float>(blockZ) + 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f));

    return playerBox.getMin().x < blockBox.getMax().x && playerBox.getMax().x > blockBox.getMin().x &&
           playerBox.getMin().y < blockBox.getMax().y && playerBox.getMax().y > blockBox.getMin().y &&
           playerBox.getMin().z < blockBox.getMax().z && playerBox.getMax().z > blockBox.getMin().z;
}

Block::BlockType CollisionDetector::getBlockAtWorldPosition(const int worldX, const int worldY, const int worldZ,
    const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk> > &loadedChunks) {
    const int chunkX = static_cast<int>(std::floor(static_cast<float>(worldX) / static_cast<float>(Chunk::SIZE))) *
                       static_cast<int>(Chunk::SIZE);
    const int chunkY = static_cast<int>(std::floor(static_cast<float>(worldY) / static_cast<float>(Chunk::SIZE))) *
                       static_cast<int>(Chunk::SIZE);
    const int chunkZ = static_cast<int>(std::floor(static_cast<float>(worldZ) / static_cast<float>(Chunk::SIZE))) *
                       static_cast<int>(Chunk::SIZE);

    const ChunkPosition chunkPos{chunkX, chunkY, chunkZ};

    const auto it = loadedChunks.find(chunkPos);
    if (it == loadedChunks.end()) {
        return Block::BlockType::STONE; // Return a solid block type if the chunk is not loaded
    }

    const int localX = worldX - chunkX;
    const int localY = worldY - chunkY;
    const int localZ = worldZ - chunkZ;

    if (localX < 0 || localX >= static_cast<int>(Chunk::SIZE) ||
        localY < 0 || localY >= static_cast<int>(Chunk::SIZE) ||
        localZ < 0 || localZ >= static_cast<int>(Chunk::SIZE)) {
        return Block::BlockType::AIR;
    }

    return it->second->getBlockType(localX, localY, localZ);
}

bool CollisionDetector::isSolidBlock(const Block::BlockType blockType) {
    switch (blockType) {
        using enum Block::BlockType;
        case AIR:
        case WATER:
        case SHORT_GRASS:
        case FLOWER_POPPY:
        case FLOWER_CORNFLOWER:
        case FLOWER_ALLIUM:
            return false;
        default:
            return true;
    }
}
