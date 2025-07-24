#include "World.h"

#include <iostream>
#include <ranges>
#include <thread>

#include "../render/Camera.h"
#include "../render/Renderer.h"
#include "../utils/ThreadSafeQueue.h"


World::World() : m_threadPool(std::max(1u, std::thread::hardware_concurrency())) {
    m_noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseGenerator.SetFrequency(.0055f);
    m_noiseGenerator.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseGenerator.SetFractalOctaves(6);
    m_noiseGenerator.SetFractalLacunarity(2.2f);

    m_loadedChunks.reserve(16 * 16 * 16); // Reserve space for 4096 chunks initially
}

World::~World() {
    m_loadedChunks.clear();
}

void World::updateChunks(Camera &camera, const float renderDistanceInBlocks) {
    if (!camera.hasCameraChangedChunk()) return;

    const int chunkSize = static_cast<int>(Chunk::m_size1());
    const int cameraWorldX = static_cast<int>(std::floor(camera.m_camera_pos().x / static_cast<float>(chunkSize))) * chunkSize;
    const int cameraWorldY = static_cast<int>(std::floor(camera.m_camera_pos().y / static_cast<float>(chunkSize))) * chunkSize;
    const int cameraWorldZ = static_cast<int>(std::floor(camera.m_camera_pos().z / static_cast<float>(chunkSize))) * chunkSize;
    const glm::vec3 cameraChunkPos(cameraWorldX, cameraWorldY, cameraWorldZ);

    unloadDistantChunks(cameraChunkPos, static_cast<int>(renderDistanceInBlocks));

    // First pass: generate voxel data for each chunk
    generateVoxelDataForEachChunks(renderDistanceInBlocks, cameraWorldX, cameraWorldY, cameraWorldZ, cameraChunkPos);

    // Second pass: generate mesh data for each chunk
    generateMeshDataForEachChunks(cameraChunkPos, renderDistanceInBlocks);
}

const FastNoiseLite & World::m_noise_generator() const {
    return m_noiseGenerator;
}

const std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> & World::m_loaded_chunks() const {
    return m_loadedChunks;
}

void World::generateVoxelDataForEachChunks(const float renderDistanceInBlocks, const int cameraWorldX,
                                          const int cameraWorldY, const int cameraWorldZ, const glm::vec3 &cameraChunkPos) {
    std::vector<std::tuple<int, int, int>> chunksToProcess;
    const int r = static_cast<int>(renderDistanceInBlocks / static_cast<float>(Chunk::m_size1()));
    const int r2 = r * r;

    const auto t1 = std::chrono::high_resolution_clock::now();
    // Generate chunks in a spherical area
    for (int x = -r; x <= r; x++) {
        const int dx2 = x * x;
        const int maxZ = static_cast<int>(std::floor(std::sqrt(static_cast<float>(r2 - dx2))));
        const int chunkX = cameraWorldX + static_cast<int>(x * Chunk::m_size1());

        for (int z = -maxZ; z <= maxZ; z++) {
            const int dz2 = z * z;
            const int rem = r2 - dx2 - dz2;
            const int maxY = static_cast<int>(std::floor(std::sqrt(static_cast<float>(rem))));
            const int chunkZ = cameraWorldZ + static_cast<int>(z * Chunk::m_size1());

            for (int y = -maxY; y <= maxY; y++) {
                const int chunkY = cameraWorldY + static_cast<int>(y * Chunk::m_size1());
                if (chunkY < 0 || chunkY > 256) continue; // World height limit
                chunksToProcess.emplace_back(chunkX, chunkY, chunkZ);
            }
        }
    }

    for (const auto& [chunkX, chunkY, chunkZ] : chunksToProcess) {
        const std::tuple<int, int, int> existingChunkKey = std::make_tuple(chunkX, chunkY, chunkZ);
        std::shared_ptr<Chunk> chunk_ptr;
        {
            std::lock_guard lock(m_chunksMutex);
            if (m_loadedChunks.contains(existingChunkKey)) continue;
            chunk_ptr = std::make_shared<Chunk>(chunkX, chunkY, chunkZ);
            m_loadedChunks[existingChunkKey] = chunk_ptr;
        }
        m_threadPool.enqueue([=, this] {
            chunk_ptr->generateVoxelData(m_noiseGenerator);
        });
    }

    const auto t2 = std::chrono::high_resolution_clock::now();
    if (const auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1); ms_int.count() > 0) {
        std::cout << "[generateVoxelData] " << ms_int.count() << "ms\n";
    }
}

void World::generateMeshDataForEachChunks(const glm::vec3 &cameraChunkPos, const float renderDistanceInBlocks) {
    const auto t1 = std::chrono::high_resolution_clock::now();
    {
        std::lock_guard lock(m_chunksMutex);
        for (const auto &chunk: m_loadedChunks | std::views::values) {
            if (const glm::vec3 chunkPos(chunk->m_x1(), chunk->m_y1(), chunk->m_z1());
                glm::distance(cameraChunkPos, chunkPos) > renderDistanceInBlocks - static_cast<float>(Chunk::m_size1())) {
                continue;
            }

            if (chunk->m_status1() == Status::VOXEL_GENERATED) {
                m_threadPool.enqueue([chunk, this] {
                    chunk->generateMeshData();
                    if (chunk->hasVisibleFaces()) m_chunksToRender.push(chunk);
                });
            }
        }
    }

    const auto t2 = std::chrono::high_resolution_clock::now();
    if (const auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1); ms_int.count() > 0) {
        std::cout << "[generateMeshData] " << ms_int.count() << "ms\n";
    }
}

void World::unloadDistantChunks(const glm::vec3 &cameraChunkPos, const int renderDistance) {
    std::lock_guard lock(m_chunksMutex);
    for (auto it = m_loadedChunks.begin(); it != m_loadedChunks.end();) {
        const std::shared_ptr<Chunk> chunk = it->second;
        if (glm::vec3 chunkPos(chunk->m_x1(), chunk->m_y1(), chunk->m_z1());
            glm::distance(cameraChunkPos, chunkPos) > static_cast<float>(renderDistance)) {
            it = m_loadedChunks.erase(it);
        } else {
            ++it;
        }
    }
}
