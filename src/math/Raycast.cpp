#include "Raycast.h"

#include <array>
#include <cfloat>
#include <cmath>

#include "common.hpp"

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

std::array<int, 3> Raycast::castRay(const glm::vec3 &rayStart, const glm::vec3 &rayDir,
                                    const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk> > &chunks) {
    // DDA Algorithm
    glm::vec3 rayUnitStepSize;
    rayUnitStepSize.x = rayDir.x == 0.f ? FLT_MAX : std::abs(1.f / rayDir.x);
    rayUnitStepSize.y = rayDir.y == 0.f ? FLT_MAX : std::abs(1.f / rayDir.y);
    rayUnitStepSize.z = rayDir.z == 0.f ? FLT_MAX : std::abs(1.f / rayDir.z);
    glm::vec3 rayCurrentPos = glm::floor(rayStart);
    glm::vec3 rayLength1D = {0.f, 0.f, 0.f};
    glm::vec3 step;

    // Starting point on x, y, z axes
    for (int i = 0; i < 3; ++i) {
        if (rayDir[i] < 0.f) {
            step[i] = -1.f;
            rayLength1D[i] = (rayStart[i] - rayCurrentPos[i]) * rayUnitStepSize[i];
        } else {
            step[i]= 1.f;
            rayLength1D[i]= (rayCurrentPos[i] + 1.f - rayStart[i]) * rayUnitStepSize[i];
        }
    }

    // Walk until collision or maximum distance
    constexpr float MAX_DISTANCE = 5.f;
    float distance = 0.f;
    while (distance < MAX_DISTANCE) {
        if (rayLength1D.x < rayLength1D.y && rayLength1D.x < rayLength1D.z) {
            rayCurrentPos.x += step.x;
            distance = rayLength1D.x;
            rayLength1D.x += rayUnitStepSize.x;
        } else if (rayLength1D.y < rayLength1D.x && rayLength1D.y < rayLength1D.z) {
            rayCurrentPos.y += step.y;
            distance = rayLength1D.y;
            rayLength1D.y += rayUnitStepSize.y;
        } else {
            rayCurrentPos.z += step.z;
            distance = rayLength1D.z;
            rayLength1D.z += rayUnitStepSize.z;
        }

        // Check collision
        // Chunk coordinates
        const int chunkX = static_cast<int>(std::floor(rayCurrentPos.x / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
        const int chunkY = static_cast<int>(std::floor(rayCurrentPos.y / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
        const int chunkZ = static_cast<int>(std::floor(rayCurrentPos.z / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);

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
        const int worldX = static_cast<int>(rayCurrentPos.x);
        const int worldY = static_cast<int>(rayCurrentPos.y);
        const int worldZ = static_cast<int>(rayCurrentPos.z);

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

void Raycast::clearCache() {
    m_cachedChunks[0] = nullptr;
    m_cachedChunks[1] = nullptr;
    m_cachedChunks[2] = nullptr;
}
