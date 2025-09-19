#include "Raycast.h"

#include <array>
#include <cfloat>
#include <cmath>

#include "glm/common.hpp"

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

RaycastResult Raycast::castRay(const glm::vec3 &rayStart, const glm::vec3 &rayDir,
                               const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk> > &chunks) {
    // DDA Algorithm - Initialize
    const glm::vec3 rayUnitStepSize = {
        rayDir.x == 0.f ? FLT_MAX : std::abs(1.f / rayDir.x),
        rayDir.y == 0.f ? FLT_MAX : std::abs(1.f / rayDir.y),
        rayDir.z == 0.f ? FLT_MAX : std::abs(1.f / rayDir.z)
    };

    glm::vec3 rayCurrentPos = glm::floor(rayStart);
    glm::vec3 rayLength1D = {0.f, 0.f, 0.f};
    glm::vec3 step;

    // Calculate initial step and rayLength1D
    for (int i = 0; i < 3; ++i) {
        if (rayDir[i] < 0.f) {
            step[i] = -1.f;
            rayLength1D[i] = (rayStart[i] - rayCurrentPos[i]) * rayUnitStepSize[i];
        } else {
            step[i] = 1.f;
            rayLength1D[i] = (rayCurrentPos[i] + 1.f - rayStart[i]) * rayUnitStepSize[i];
        }
    }

    // Walk until collision or maximum distance
    constexpr float MAX_DISTANCE = 5.f;
    glm::ivec3 hitNormal;
    float distance = 0.f;
    while (distance < MAX_DISTANCE) {
        // Update ray position and distance
        updateRayStep(rayCurrentPos, distance, rayLength1D, step, rayUnitStepSize, hitNormal);

        const glm::ivec3 chunkCoords = calculateChunkCoordinates(rayCurrentPos);

        const int chunkIndex = findOrCacheChunk(chunkCoords, chunks);
        if (chunkIndex == -1) {
            continue; // Chunk not found
        }

        const glm::ivec3 worldCoords = {
            static_cast<int>(rayCurrentPos.x),
            static_cast<int>(rayCurrentPos.y),
            static_cast<int>(rayCurrentPos.z)
        };
        const glm::ivec3 localCoords = worldCoords - chunkCoords;

        // Check for block collision
        if (Block::BlockType blockType;
            checkBlockCollision(chunkIndex, localCoords, blockType)) {
            return {
                m_cachedChunks[chunkIndex],
                localCoords.x,
                localCoords.y,
                localCoords.z,
                worldCoords,
                hitNormal,
                true,
                blockType
            };
        }
    }

    static RaycastResult result;
    result.hasHitBlock = false;
    return result;
}

glm::ivec3 Raycast::calculateChunkCoordinates(const glm::vec3& rayCurrentPos) {
    const int chunkX = static_cast<int>(std::floor(rayCurrentPos.x / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
    const int chunkY = static_cast<int>(std::floor(rayCurrentPos.y / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
    const int chunkZ = static_cast<int>(std::floor(rayCurrentPos.z / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
    return {chunkX, chunkY, chunkZ};
}

int Raycast::findOrCacheChunk(const glm::ivec3& chunkCoords,
                           const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>>& chunks) {
    const int chunkX = chunkCoords.x, chunkY = chunkCoords.y, chunkZ = chunkCoords.z;

    // Check cached chunks
    if (m_lastChunk0X == chunkX && m_lastChunk0Y == chunkY && m_lastChunk0Z == chunkZ) {
        return 0;
    }
    if (m_lastChunk1X == chunkX && m_lastChunk1Y == chunkY && m_lastChunk1Z == chunkZ) {
        return 1;
    }
    if (m_lastChunk2X == chunkX && m_lastChunk2Y == chunkY && m_lastChunk2Z == chunkZ) {
        return 2;
    }

    // Find chunk in map
    const auto it = chunks.find({chunkX, chunkY, chunkZ});
    if (it == chunks.end()) {
        // Invalidate cached chunks if the chunk is not found
        m_lastChunk0X = m_lastChunk0Y = m_lastChunk0Z = -1;
        m_lastChunk1X = m_lastChunk1Y = m_lastChunk1Z = -1;
        m_lastChunk2X = m_lastChunk2Y = m_lastChunk2Z = -1;
        return -1; // Not found
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

    return 0;
}

bool Raycast::checkBlockCollision(const int chunkIndex, const glm::ivec3& localCoords, Block::BlockType& outBlockType) {
    const Block::BlockType block = m_cachedChunks[chunkIndex]->getBlockTypeOrSurfaceFeature(
        localCoords.x, localCoords.y, localCoords.z);
    outBlockType = block;
    return !(block == Block::BlockType::AIR || block == Block::BlockType::WATER);
}

void Raycast::updateRayStep(glm::vec3& rayCurrentPos, float& distance, glm::vec3& rayLength1D,
                         const glm::vec3& step, const glm::vec3& rayUnitStepSize, glm::ivec3& hitNormal) {
    if (rayLength1D.x < rayLength1D.y && rayLength1D.x < rayLength1D.z) {
        rayCurrentPos.x += step.x;
        distance = rayLength1D.x;
        rayLength1D.x += rayUnitStepSize.x;
        hitNormal = {-step.x, 0, 0};
    } else if (rayLength1D.y < rayLength1D.z) {
        rayCurrentPos.y += step.y;
        distance = rayLength1D.y;
        rayLength1D.y += rayUnitStepSize.y;
        hitNormal = {0, -step.y, 0};
    } else {
        rayCurrentPos.z += step.z;
        distance = rayLength1D.z;
        rayLength1D.z += rayUnitStepSize.z;
        hitNormal = {0, 0, -step.z};
    }
}
