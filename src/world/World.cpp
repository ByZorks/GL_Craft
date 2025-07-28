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
        Renderer::m_renderDistance * Renderer::m_renderDistance * Renderer::m_renderDistance * 0.5f)); // Estimation based on testing
}

World::~World() {
    m_loadedChunks.clear();
    m_chunksToGenerate.clear();
    m_chunksToDelete.clear();
    m_chunksToRender.clear();
}

void World::updateChunks(Camera &camera, const float renderDistanceInBlocks) {
    if (!camera.hasCameraChangedChunk()) return;

    const int cameraWorldX = static_cast<int>(std::floor(camera.m_camera_pos().x / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
    const int cameraWorldY = static_cast<int>(std::floor(camera.m_camera_pos().y / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
    const int cameraWorldZ = static_cast<int>(std::floor(camera.m_camera_pos().z / static_cast<float>(Chunk::SIZE))) * static_cast<int>(Chunk::SIZE);
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
    const int r = static_cast<int>(renderDistanceInBlocks / static_cast<float>(Chunk::SIZE));
    const int r2 = r * r;

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
        int chunkX = cameraWorldX + static_cast<int>(x * Chunk::SIZE);
        int chunkZ = cameraWorldZ + static_cast<int>(z * Chunk::SIZE);

        for (int y = -maxY; y <= maxY; y++) {
            const int chunkY = cameraWorldY + static_cast<int>(y * Chunk::SIZE);
            if (chunkY < 0 || chunkY > 256) continue; // World height limit

            const std::tuple<int, int, int> key = std::make_tuple(chunkX, chunkY, chunkZ);
            if (m_loadedChunks.contains(key)) continue;
            m_chunksToGenerate.push(key);
        }
    }
}

void World::unloadDistantChunks(const glm::vec3 &cameraChunkPos, const float renderDistance) {
    std::erase_if(m_loadedChunks, [&](const auto &pair) {
        auto [x, y, z] = pair.first;
        return glm::distance(glm::vec3(x, y, z), cameraChunkPos) > renderDistance;
    });
}
