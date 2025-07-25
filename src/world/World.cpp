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

    m_loadedChunks.reserve(static_cast<std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>>::size_type>(
        Renderer::m_renderDistance * Renderer::m_renderDistance * Renderer::m_renderDistance) * 2);
}

World::~World() {
    m_loadedChunks.clear();
    while (m_chunksToRender.pop());
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
    // generateMeshDataForEachChunks(cameraChunkPos, renderDistanceInBlocks);
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
    // Pre-compute offsets for a circle of chunks around the camera position
    struct Offset { int x, z, maxY; };
    std::vector<Offset> circleOffsets;
    circleOffsets.reserve(static_cast<std::vector<Offset>::size_type>(std::numbers::pi * static_cast<double>(r2)));
    for (int x = -r; x <= r; x++) {
        for (int z = -r; z <= r; z++) {
            if (const int d2 = x*x + z*z; d2 <= r2) {
                circleOffsets.push_back({x, z,
                    static_cast<int>(std::floor(std::sqrt(static_cast<float>(r2 - d2))))});
            }
        }
    }


    // Sort offsets by distance
    std::sort(circleOffsets.begin(), circleOffsets.end(),
              [&](auto &a, auto &b) {
                  return a.x*a.x + a.z*a.z
                   < b.x*b.x + b.z*b.z;
              });

    // Generate chunks
    for (auto [x,z, maxY] : circleOffsets) {
        int chunkX = cameraWorldX + static_cast<int>(x * Chunk::m_size1());
        int chunkZ = cameraWorldZ + static_cast<int>(z * Chunk::m_size1());

        for (int y = -maxY; y <= maxY; y++) {
            const int chunkY = cameraWorldY + static_cast<int>(y * Chunk::m_size1());
            if (chunkY < 0 || chunkY > 256) continue; // World height limit

            const std::tuple<int, int, int> existingChunkKey = std::make_tuple(chunkX, chunkY, chunkZ);
            std::shared_ptr<Chunk> p_chunk;
            {
                std::lock_guard lock(m_chunksMutex);
                if (m_loadedChunks.contains(existingChunkKey)) continue;
                p_chunk = std::make_shared<Chunk>(chunkX, chunkY, chunkZ);
                m_loadedChunks[existingChunkKey] = p_chunk;
            }
            m_threadPool.enqueue([=, this] {
                p_chunk->generateVoxelData(m_noiseGenerator);
                p_chunk->generateMeshData();
                if (p_chunk->hasVisibleFaces()) m_chunksToRender.push(p_chunk);
            });
        }
    }

    const auto t2 = std::chrono::high_resolution_clock::now();
    if (const auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1); ms_int.count() > 0) {
        std::cout << "[generateVoxelData] " << ms_int.count() << "ms\n";
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
