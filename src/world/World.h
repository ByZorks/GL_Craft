#ifndef WORLD_H
#define WORLD_H

#include <mutex>
#include <ranges>
#include <unordered_map>

#include "Chunk.h"

#include "FastNoiseLite.h"
#include "../math/Frustum.h"
#include "../render/Renderer.h"
#include "../render/ThreadPool.h"
#include "../utils/CustomHash.h"
#include "../utils/ThreadSafeQueue.h"

class Camera;

class World {
private:
    ThreadPool m_threadPool;
    mutable std::mutex m_chunksMutex;
    std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> m_loadedChunks;
    ThreadSafeQueue<std::tuple<int, int, int>> m_chunksToRender;
    ThreadSafeQueue<std::shared_ptr<Chunk>> m_chunksToDelete;
    FastNoiseLite m_terrainHeightGenerator;
    FastNoiseLite m_caveGenerator;

public:
    World();
    ~World();

    void updateChunks(Camera &camera, float renderDistanceInBlocks);
    template<typename Callback>
    void forEachRenderableChunk(Callback&& callback);

    [[nodiscard]] const FastNoiseLite & m_noise_generator() const;
    [[nodiscard]] const std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> & m_loaded_chunks() const;

private:
    void generateDataForEachChunks(float renderDistanceInBlocks, int cameraWorldX, int cameraWorldY, int cameraWorldZ);
    void unloadDistantChunks(const glm::vec3 &cameraChunkPos, float renderDistance);
};

template<typename Callback>
void World::forEachRenderableChunk(Callback &&callback) {
    const int maxRemovePerFrame = static_cast<int>((Renderer::m_renderDistance + static_cast<float>(m_threadPool.m_num_threads())) / 5);
    // Remove chunks that are no longer needed
    for (int i = 0; i < maxRemovePerFrame; ++i) {
        if (!m_chunksToDelete.empty()) m_chunksToDelete.pop();
    }

    // Process chunks that are within the render distance
    const int maxChunksPerFrame = static_cast<int>((Renderer::m_renderDistance + static_cast<float>(m_threadPool.m_num_threads())) / 10);
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksToRender.empty()) break;

        std::tuple<int, int, int> key = m_chunksToRender.pop();
        m_threadPool.enqueue([this, key] {
            auto p_chunk = std::make_shared<Chunk>(std::get<0>(key), std::get<1>(key), std::get<2>(key));
            p_chunk->generateVoxelData(m_terrainHeightGenerator, m_caveGenerator);
            if (!p_chunk->hasBlocks()) { // perhaps not worth it
                m_chunksToDelete.push(p_chunk);
                return;
            }
            p_chunk->generateMeshData();
            if (!p_chunk->hasVisibleFaces()) {
                m_chunksToDelete.push(p_chunk);
                return;
            }
            {
                std::lock_guard lock(m_chunksMutex);
                m_loadedChunks.try_emplace(key, p_chunk);
            }
        });
    }

    // Setup buffers for loaded chunks, callback is called for each chunk that has buffers set up
    std::lock_guard lock(m_chunksMutex);
    for (const auto& chunk : m_loadedChunks | std::views::values) {
        if (chunk && chunk->m_status1() == Status::MESH_GENERATED) {
            chunk->setupBuffers();
            callback(chunk);
        }
        if (chunk && chunk->m_status1() == Status::BUFFERS_SETUP) {
            callback(chunk);
        }
    }
}

#endif //WORLD_H
