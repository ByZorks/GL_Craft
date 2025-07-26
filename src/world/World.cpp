#include "World.h"

#include <iostream>
#include <ranges>
#include <thread>

#include "../render/Camera.h"
#include "../render/Renderer.h"
#include "../utils/ThreadSafeQueue.h"


World::World() : m_threadPool(std::max(1u, std::thread::hardware_concurrency())) {
    m_terrainHeightGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_terrainHeightGenerator.SetFrequency(.0055f);
    m_terrainHeightGenerator.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_terrainHeightGenerator.SetFractalOctaves(6);
    m_terrainHeightGenerator.SetFractalLacunarity(2.2f);

    m_caveGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_caveGenerator.SetFrequency(.018f);
    m_caveGenerator.SetFractalType(FastNoiseLite::FractalType_Ridged);
    m_caveGenerator.SetFractalOctaves(6);
    m_caveGenerator.SetFractalLacunarity(1.29f);
    m_caveGenerator.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2Reduced);
    m_caveGenerator.SetDomainWarpAmp(9.f);

    m_loadedChunks.reserve(static_cast<std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>>::size_type>(
        Renderer::m_renderDistance * Renderer::m_renderDistance * Renderer::m_renderDistance) * 2);
}

World::~World() {
    m_loadedChunks.clear();
    m_chunksToRender.clear();
}

void World::updateChunks(Camera &camera, const float renderDistanceInBlocks) {
    if (!camera.hasCameraChangedChunk()) return;

    const int chunkSize = static_cast<int>(Chunk::m_size1());
    const int cameraWorldX = static_cast<int>(std::floor(camera.m_camera_pos().x / static_cast<float>(chunkSize))) * chunkSize;
    const int cameraWorldY = static_cast<int>(std::floor(camera.m_camera_pos().y / static_cast<float>(chunkSize))) * chunkSize;
    const int cameraWorldZ = static_cast<int>(std::floor(camera.m_camera_pos().z / static_cast<float>(chunkSize))) * chunkSize;
    const glm::vec3 cameraChunkPos(cameraWorldX, cameraWorldY, cameraWorldZ);

    unloadDistantChunks(cameraChunkPos, renderDistanceInBlocks);

    generateDataForEachChunks(renderDistanceInBlocks, cameraWorldX, cameraWorldY, cameraWorldZ);
}

const FastNoiseLite & World::m_noise_generator() const {
    return m_terrainHeightGenerator;
}

const std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> & World::m_loaded_chunks() const {
    return m_loadedChunks;
}

void World::generateDataForEachChunks(const float renderDistanceInBlocks, const int cameraWorldX,
                                          const int cameraWorldY, const int cameraWorldZ) {
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
                p_chunk->generateVoxelData(m_terrainHeightGenerator, m_caveGenerator);
                p_chunk->generateMeshData();
                if (!p_chunk->hasVisibleFaces()) return;
                m_chunksToRender.push(p_chunk);
            });
        }
    }

    const auto t2 = std::chrono::high_resolution_clock::now();
    if (const auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1); ms_int.count() > 0) {
        std::cout << "[generateData] " << ms_int.count() << "ms\n";
    }
}

void World::unloadDistantChunks(const glm::vec3 &cameraChunkPos, const float renderDistance) {
    std::lock_guard lock(m_chunksMutex);
    std::erase_if(m_loadedChunks, [&](const auto &pair) {
        auto [x, y, z] = pair.first;
        return glm::distance(glm::vec3(x, y, z), cameraChunkPos) > renderDistance;
    });
}
