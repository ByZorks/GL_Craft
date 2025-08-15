#include "Raycast.h"

#include <cmath>
#include <iostream>

BlockType Raycast::castRay(const glm::vec3 &pos, const glm::vec3 &dir, const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>> &chunks) {
    float currentDistance = 0.f;
    while (currentDistance < MAX_DISTANCE) {
        ++currentDistance;

        // Chunk
        const glm::vec3 currentlyCheckedPos = pos + dir * currentDistance;
        const int chunkX = static_cast<int>(std::floor(currentlyCheckedPos.x / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
        const int chunkY = static_cast<int>(std::floor(currentlyCheckedPos.y / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
        const int chunkZ = static_cast<int>(std::floor(currentlyCheckedPos.z / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
        std::shared_ptr<Chunk> p_chunk = nullptr;
        const auto it = chunks.find({chunkX, chunkY, chunkZ});
        if (it == chunks.end()) continue;
        p_chunk = it->second;

        // Local coordinates
        const int localChunkX = (static_cast<int>(currentlyCheckedPos.x) % static_cast<int>(Chunk::SIZE) + static_cast<int>(Chunk::SIZE)) % static_cast<int>(Chunk::SIZE);
        const int localChunkY = (static_cast<int>(currentlyCheckedPos.y) % static_cast<int>(Chunk::SIZE) + static_cast<int>(Chunk::SIZE)) % static_cast<int>(Chunk::SIZE);
        const int localChunkZ = (static_cast<int>(currentlyCheckedPos.z) % static_cast<int>(Chunk::SIZE) + static_cast<int>(Chunk::SIZE)) % static_cast<int>(Chunk::SIZE);

        // Get block type
        const BlockType block = p_chunk->getBlockType(localChunkX, localChunkY, localChunkZ);
        if (!(block == BlockType::AIR || block == BlockType::WATER)) {
            std::cout << "[Raycast] Block found at (" << localChunkX << ", " << localChunkY << ", " << localChunkZ << ") with type: " << static_cast<int>(block) << std::endl;
            std::cout << "[Raycast] Distance: " << currentDistance << std::endl;
            return block;
        }
    }

    return BlockType::AIR;
}
