#include "Raycast.h"

#include <array>
#include <cmath>
#include <iostream>

int Raycast::m_lastChunk0X = -1;
int Raycast::m_lastChunk0Y = -1;
int Raycast::m_lastChunk0Z = -1;
int Raycast::m_lastChunk1X = -1;
int Raycast::m_lastChunk1Y = -1;
int Raycast::m_lastChunk1Z = -1;
int Raycast::m_lastChunk2X = -1;
int Raycast::m_lastChunk2Y = -1;
int Raycast::m_lastChunk2Z = -1;
std::array<std::shared_ptr<Chunk>, 3> Raycast::m_cachedChunks = {nullptr, nullptr, nullptr};

std::array<int, 3> Raycast::castRay(const glm::vec3 &pos, const glm::vec3 &dir,
                                    const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk> > &chunks) {
    float currentDistance = 0.f;
    while (currentDistance < MAX_DISTANCE) {
        currentDistance += EPSILON;

        // Chunk coordinates
        const glm::vec3 currentlyCheckedPos = pos + dir * currentDistance;
        const int chunkX = static_cast<int>(std::floor(currentlyCheckedPos.x / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
        const int chunkY = static_cast<int>(std::floor(currentlyCheckedPos.y / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
        const int chunkZ = static_cast<int>(std::floor(currentlyCheckedPos.z / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);

        int chunkIndex;
        if (m_lastChunk0X == chunkX && m_lastChunk0Y == chunkY && m_lastChunk0Z == chunkZ) {
            chunkIndex = 0;
        } else if (m_lastChunk1X == chunkX && m_lastChunk1Y == chunkY && m_lastChunk1Z == chunkZ) {
            chunkIndex = 1;
        } else if (m_lastChunk2X == chunkX && m_lastChunk2Y == chunkY && m_lastChunk2Z == chunkZ) {
            chunkIndex = 2;
        } else {
            const auto it = chunks.find({chunkX, chunkY, chunkZ});
            if (it == chunks.end()) {
                // Invalidate cached chunks if the chunk is not found
                m_lastChunk0X = m_lastChunk0Y = m_lastChunk0Z = -1;
                m_lastChunk1X = m_lastChunk1Y = m_lastChunk1Z = -1;
                m_lastChunk2X = m_lastChunk2Y = m_lastChunk2Z = -1;
                continue;
            }

            // Shift cached chunks
            m_cachedChunks[2] = m_cachedChunks[1];
            m_lastChunk2X = m_lastChunk1X;
            m_lastChunk2Y = m_lastChunk1Y;
            m_lastChunk2Z = m_lastChunk1Z;

            m_cachedChunks[1] = m_cachedChunks[0];
            m_lastChunk1X = m_lastChunk0X;
            m_lastChunk1Y = m_lastChunk0Y;
            m_lastChunk1Z = m_lastChunk0Z;

            m_cachedChunks[0] = it->second;
            m_lastChunk0X = chunkX;
            m_lastChunk0Y = chunkY;
            m_lastChunk0Z = chunkZ;

            chunkIndex = 0;
        }

        // World coordinates
        const int worldX = static_cast<int>(std::floor(currentlyCheckedPos.x));
        const int worldY = static_cast<int>(std::floor(currentlyCheckedPos.y));
        const int worldZ = static_cast<int>(std::floor(currentlyCheckedPos.z));

        // Local coordinates
        const int localChunkX = worldX - chunkX;
        const int localChunkY = worldY - chunkY;
        const int localChunkZ = worldZ - chunkZ;

        // Get block type
        if (const BlockType block = m_cachedChunks[chunkIndex]->getBlockType(localChunkX, localChunkY, localChunkZ);
            !(block == BlockType::AIR || block == BlockType::WATER)) {
            return {worldX, worldY, worldZ};
        }
    }

    return {0, 0, 0}; // No block found within the maximum distance
}
