#include "World.h"

#include <iostream>
#include <ranges>

#include "../render/Camera.h"

World::World() {
    m_noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseGenerator.SetFrequency(.01f);
    m_noiseGenerator.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseGenerator.SetFractalOctaves(4);
}

World::~World() {
    for (const auto &chunk: m_loadedChunks | std::views::values) {
        delete chunk;
    }
    m_loadedChunks.clear();
}

Chunk * World::getChunk(int chunkBaseX, int chunkBaseY, int chunkBaseZ) const {
    if (const auto key = std::make_tuple(chunkBaseX, chunkBaseY, chunkBaseZ); m_loadedChunks.contains(key)) {
        return m_loadedChunks.at(key);
    }
    return nullptr;
}

void World::updateChunks(const Camera &camera, const float renderDistanceInBlocks) {
    const int chunkSize = static_cast<int>(Chunk::m_size1());
    const int renderDistanceInChunks = static_cast<int>(renderDistanceInBlocks) / chunkSize + 1; // +1 to include the next chunk after the render distance
    const glm::vec3 cameraPos = camera.m_camera_pos();

    // Calculate which chunk the camera is in
    const int cameraChunkX = static_cast<int>(std::floor(cameraPos.x / static_cast<float>(chunkSize)));
    const int cameraChunkZ = static_cast<int>(std::floor(cameraPos.z / static_cast<float>(chunkSize)));

    if (m_lastCameraChunkPos.x == static_cast<float>(cameraChunkX) && m_lastCameraChunkPos.y == static_cast<float>(cameraChunkZ)) {
        return; // Camera hasn't moved to a new chunk, no need to update
    }
    m_lastCameraChunkPos = {cameraChunkX, cameraChunkZ};

    const int cameraWorldX = cameraChunkX * chunkSize;
    const int cameraWorldZ = cameraChunkZ * chunkSize;
    const glm::vec3 cameraChunkPos(cameraWorldX, 0, cameraWorldZ);

    unloadDistantChunks(cameraChunkPos, static_cast<int>(renderDistanceInBlocks));

    // First pass: generate voxel data for each chunk
    for (int x = -renderDistanceInChunks; x <= renderDistanceInChunks; x++) {
        for (int z = -renderDistanceInChunks; z <= renderDistanceInChunks; z++) {
            const int chunkX = cameraWorldX + x * chunkSize;
            const int chunkZ = cameraWorldZ + z * chunkSize;

            constexpr int maxChunkY = 100;

            for (int y = 0; y * chunkSize <= maxChunkY; y++) {
                const int chunkY = y * chunkSize;
                const glm::vec3 chunkPos(chunkX, chunkY, chunkZ);

                // Check if the chunk is within the render distance
                if (const float distance = glm::distance(cameraChunkPos, chunkPos); distance > renderDistanceInBlocks) {
                    continue;
                }

                const std::tuple<int, int, int> existingChunkKey = std::make_tuple(chunkX, chunkY, chunkZ);
                if (m_loadedChunks.contains(existingChunkKey)) {
                    continue;
                }

                auto* chunk = new Chunk(chunkX, chunkY, chunkZ);
                chunk->generateVoxelData(m_noiseGenerator);
                m_loadedChunks[existingChunkKey] = chunk;
            }
        }
    }

    // Second pass: generate mesh data for each chunk and set up buffers
    for (const auto &chunk: m_loadedChunks | std::views::values) {
        const glm::vec3 chunkPos(chunk->m_x_start(), chunk->m_y_start(), chunk->m_z_start());
        if (const float distance = glm::distance(cameraChunkPos, chunkPos); distance > renderDistanceInBlocks - static_cast<float>(chunkSize)) {
            continue;
        }

        if (chunk->m_status1() >= MESH_GENERATED) continue;
        chunk->generateMeshData(this);
        if (chunk->m_status1() >= BUFFERS_SETUP) continue;
        chunk->setupBuffers();
    }
}

std::unordered_map<std::tuple<int, int, int>, Chunk *> & World::m_chunks1() {
    return m_loadedChunks;
}

void World::unloadDistantChunks(const glm::vec3 cameraChunkPos, const int renderDistance) {
    const auto maxDistance = static_cast<float>(renderDistance);

    for (auto it = m_loadedChunks.begin(); it != m_loadedChunks.end();) {
        const Chunk* chunk = it->second;
        if (glm::vec3 chunkPos(chunk->m_x_start(), chunk->m_y_start(), chunk->m_z_start()); glm::distance(cameraChunkPos, chunkPos) > maxDistance) {
            delete chunk;
            it = m_loadedChunks.erase(it);
        } else {
            ++it;
        }
    }
}
