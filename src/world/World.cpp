#include "World.h"

#include <iostream>
#include <ranges>
#include <thread>

#include "../render/Camera.h"
#include "../utils/ThreadSafeQueue.h"

World::World(): m_threadPool(std::max(1u, std::thread::hardware_concurrency())) {
    m_noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseGenerator.SetFrequency(.007f);
    m_noiseGenerator.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseGenerator.SetFractalOctaves(6);

    m_loadedChunks.reserve(16 * 16 * 16); // Reserve space for 4096 chunks initially
}

World::~World() {
    m_loadedChunks.clear();
}

std::shared_ptr<Chunk> World::getChunk(int chunkBaseX, int chunkBaseY, int chunkBaseZ) const {
    std::lock_guard lock(m_chunksMutex);
    if (const auto key = std::make_tuple(chunkBaseX, chunkBaseY, chunkBaseZ); m_loadedChunks.contains(key)) {
        return m_loadedChunks.at(key);
    }
    return nullptr;
}

void World::updateChunks(const Camera &camera, const float renderDistanceInBlocks) {
    // Calculate which chunk the camera is in
    const int cameraChunkX = camera.m_camera_pos().x / Chunk::m_size1();
    const int cameraChunkY = camera.m_camera_pos().y / Chunk::m_size1();
    const int cameraChunkZ = camera.m_camera_pos().z / Chunk::m_size1();

    if (m_lastCameraChunkPos.x == cameraChunkX &&
        m_lastCameraChunkPos.y == cameraChunkY &&
        m_lastCameraChunkPos.z == cameraChunkZ) {
        return; // Camera hasn't moved to a new chunk, no need to update
    }
    m_lastCameraChunkPos = {cameraChunkX, cameraChunkY, cameraChunkZ};

    const int cameraWorldX = cameraChunkX * Chunk::m_size1();
    const int cameraWorldY = cameraChunkY * Chunk::m_size1();
    const int cameraWorldZ = cameraChunkZ * Chunk::m_size1();
    const glm::vec3 cameraChunkPos(cameraWorldX, cameraWorldY, cameraWorldZ);

    unloadDistantChunks(cameraChunkPos, static_cast<int>(renderDistanceInBlocks));

    // First pass: generate voxel data for each chunk
    const int renderDistanceInChunks = renderDistanceInBlocks / Chunk::m_size1();
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int x = -renderDistanceInChunks; x <= renderDistanceInChunks; x++) {
        const int chunkX = cameraWorldX + x * Chunk::m_size1();

        for (int z = -renderDistanceInChunks; z <= renderDistanceInChunks; z++) {
            const int chunkZ = cameraWorldZ + z * Chunk::m_size1();

            for (int y = -renderDistanceInChunks; y <= renderDistanceInChunks; y++) {
                const int chunkY = cameraWorldY + y * Chunk::m_size1();
                if (chunkY < 0 || chunkY > 200) continue; // World height limit
                const glm::vec3 chunkPos(chunkX, chunkY, chunkZ);

                // Check if the chunk is within the render distance
                if (glm::distance(cameraChunkPos, chunkPos) > renderDistanceInBlocks) continue;

                // Check if the chunk already exists
                const std::tuple<int, int, int> existingChunkKey = std::make_tuple(chunkX, chunkY, chunkZ);
                bool chunkExists = false;
                {
                    std::lock_guard lock(m_chunksMutex);
                    chunkExists = m_loadedChunks.contains(existingChunkKey);
                }
                if (chunkExists) continue;

                // Create a new chunk if it doesn't exist
                const auto chunk_ptr = std::make_shared<Chunk>(chunkX, chunkY, chunkZ);
                m_threadPool.enqueue([=, this] {
                    {
                        std::lock_guard lock(m_chunksMutex);
                        m_loadedChunks[existingChunkKey] = chunk_ptr;
                    }
                    chunk_ptr->generateVoxelData(m_noiseGenerator);
                });
            }
        }
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    if (ms_int.count() > 0) {
        std::cout << "[generateVoxelData] " << ms_int.count() << "ms\n";
    }

    // Second pass: generate mesh data for each chunk
    t1 = std::chrono::high_resolution_clock::now();
    {
        std::lock_guard lock(m_chunksMutex);
        for (const auto &chunk: m_loadedChunks | std::views::values) {
            if (const glm::vec3 chunkPos(chunk->m_x_start(), chunk->m_y_start(), chunk->m_z_start());
                glm::distance(cameraChunkPos, chunkPos) > renderDistanceInBlocks - Chunk::m_size1()) {
                continue;
            }

            if (chunk->m_status1() == Status::VOXEL_GENERATED) {
                m_threadPool.enqueue([chunk, this] {
                    chunk->generateMeshData();
                    if (chunk->m_status1() < Status::BUFFERS_SETUP) {
                        m_chunksToRender.push(chunk);
                    }
                });
            }
        }
    }

    t2 = std::chrono::high_resolution_clock::now();
    ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    if (ms_int.count() > 0) {
        std::cout << "[generateMeshData] " << ms_int.count() << "ms\n";
    }
}

const FastNoiseLite & World::m_noise_generator() const {
    return m_noiseGenerator;
}

const std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> & World::m_loaded_chunks() const {
    return m_loadedChunks;
}

void World::unloadDistantChunks(const glm::vec3 &cameraChunkPos, const int renderDistance) {
    std::lock_guard lock(m_chunksMutex);
    for (auto it = m_loadedChunks.begin(); it != m_loadedChunks.end();) {
        const std::shared_ptr<Chunk> chunk = it->second;
        if (glm::vec3 chunkPos(chunk->m_x_start(), chunk->m_y_start(), chunk->m_z_start());
            glm::distance(cameraChunkPos, chunkPos) > renderDistance) {
            it = m_loadedChunks.erase(it);
        } else {
            ++it;
        }
    }
}