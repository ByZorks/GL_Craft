#include "World.h"

#include <iostream>
#include <ranges>
#include <thread>

#include "../render/Camera.h"
#include "../utils/ThreadSafeQueue.h"

World::World() {
    m_noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseGenerator.SetFrequency(.01f);
    m_noiseGenerator.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseGenerator.SetFractalOctaves(4);

    m_loadedChunks.reserve(16 * 16 * 16); // Reserve space for 4096 chunks initially
}

World::~World() {
    for (const auto &chunk: m_loadedChunks | std::views::values) {
        delete chunk;
    }
    m_loadedChunks.clear();
}

Chunk *World::getChunk(int chunkBaseX, int chunkBaseY, int chunkBaseZ) const {
    std::lock_guard lock(m_chunksMutex);
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

    if (m_lastCameraChunkPos.x == static_cast<float>(cameraChunkX) && m_lastCameraChunkPos.y == static_cast<float>(
            cameraChunkZ)) {
        return; // Camera hasn't moved to a new chunk, no need to update
    }
    m_lastCameraChunkPos = {cameraChunkX, cameraChunkZ};

    const int cameraWorldX = cameraChunkX * chunkSize;
    const int cameraWorldZ = cameraChunkZ * chunkSize;
    const glm::vec3 cameraChunkPos(cameraWorldX, 0, cameraWorldZ);

    unloadDistantChunks(cameraChunkPos, static_cast<int>(renderDistanceInBlocks));

    const unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> threads;

    // First pass: generate voxel data for each chunk
    auto t1 = std::chrono::high_resolution_clock::now();
    ThreadSafeQueue<Chunk *> chunkQueue;
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
                bool chunkExists = false; {
                    std::lock_guard lock(m_chunksMutex);
                    chunkExists = m_loadedChunks.contains(existingChunkKey);
                }
                if (chunkExists) continue;

                auto *chunk = new Chunk(chunkX, chunkY, chunkZ); {
                    std::lock_guard lock(m_chunksMutex);
                    m_loadedChunks[existingChunkKey] = chunk;
                }
                chunkQueue.push(chunk);
            }
        }
    }

    chunkQueue.done();
    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&]() {
            while (Chunk *chunk = chunkQueue.pop()) {
                chunk->generateVoxelData(m_noiseGenerator);
            }
        });
    }

    for (auto &thread: threads) {
        if (thread.joinable()) thread.join();
    }
    threads.clear();
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    if (ms_int.count() > 0) {
        std::cout << "[generateVoxelData] " << ms_int.count() << "ms\n";
    }

    // Second pass: generate mesh data for each chunk
    t1 = std::chrono::high_resolution_clock::now();
    ThreadSafeQueue<Chunk *> meshQueue;
    std::vector<Chunk *> chunksToProcess;
    chunksToProcess.reserve(m_loadedChunks.size());
    {
        std::lock_guard lock(m_chunksMutex);
        for (const auto &chunk: m_loadedChunks | std::views::values) {
            chunksToProcess.push_back(chunk);
        }
    }

    for (const auto &chunk: chunksToProcess) {
        const glm::vec3 chunkPos(chunk->m_x_start(), chunk->m_y_start(), chunk->m_z_start());
        if (const float distance = glm::distance(cameraChunkPos, chunkPos);
            distance > renderDistanceInBlocks - static_cast<float>(chunkSize)) {
            continue;
        }

        if (chunk->m_status1() < Status::MESH_GENERATED) {
            meshQueue.push(chunk);
        }
    }

    meshQueue.done();
    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&]() {
            while (Chunk *chunk = meshQueue.pop()) {
                chunk->generateMeshData(*this);
            }
        });
    }

    for (auto &thread: threads) {
        if (thread.joinable()) thread.join();
    }
    t2 = std::chrono::high_resolution_clock::now();
    ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    if (ms_int.count() > 0) {
        std::cout << "[generateMeshData] " << ms_int.count() << "ms\n";
    }

    // Third pass: set up buffers for each chunk
    t1 = std::chrono::high_resolution_clock::now();
    {
        std::lock_guard lock(m_chunksMutex);
        for (auto *chunk: m_loadedChunks | std::views::values) {
            if (chunk->m_status1() >= Status::MESH_GENERATED
                && chunk->m_status1() < Status::BUFFERS_SETUP) {
                chunk->setupBuffers();
            }
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    if (ms_int.count() > 0) {
        std::cout << "[setupBuffers] " << ms_int.count() << "ms\n";
    }
}

const std::vector<Chunk*>& World::getChunksToRender() {
    thread_local std::vector<Chunk*> chunks;
    chunks.clear();
    std::lock_guard lock(m_chunksMutex);
    for (const auto& chunk : m_loadedChunks | std::views::values) {
        if (chunk->m_status1() == Status::BUFFERS_SETUP) {
            chunks.push_back(chunk);
        }
    }
    return chunks;
}

const FastNoiseLite & World::m_noise_generator() const {
    return m_noiseGenerator;
}

void World::unloadDistantChunks(const glm::vec3 &cameraChunkPos, const int renderDistance) {
    std::lock_guard lock(m_chunksMutex);
    const auto maxDistance = static_cast<float>(renderDistance);

    for (auto it = m_loadedChunks.begin(); it != m_loadedChunks.end();) {
        const Chunk *chunk = it->second;
        if (glm::vec3 chunkPos(chunk->m_x_start(), chunk->m_y_start(), chunk->m_z_start());
            glm::distance(cameraChunkPos, chunkPos) > maxDistance) {
            delete chunk;
            it = m_loadedChunks.erase(it);
        } else {
            ++it;
        }
    }
}

void World::processChunk(Chunk *chunk, const World *world) {
    if (chunk->m_status1() < Status::MESH_GENERATED) {
        chunk->generateMeshData(*world);
    }
    if (chunk->m_status1() < Status::BUFFERS_SETUP) {
        chunk->setupBuffers();
    }
}
