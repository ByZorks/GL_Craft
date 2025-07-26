#ifndef WORLD_H
#define WORLD_H

#include <mutex>
#include <ranges>
#include <unordered_map>

#include "Chunk.h"

#include "FastNoiseLite.h"
#include "../math/Frustum.h"
#include "../render/ThreadPool.h"
#include "../utils/CustomHash.h"
#include "../utils/ThreadSafeQueue.h"

class Camera;

class World {
private:
    ThreadPool m_threadPool;
    mutable std::mutex m_chunksMutex;
    std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> m_loadedChunks;
    ThreadSafeQueue<std::weak_ptr<Chunk>> m_chunksToRender;
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
    // Process chunks that are ready to be rendered
    constexpr int maxToProcessPerFrame = 10;
    for (int i = 0; i < maxToProcessPerFrame; ++i) {
        const std::shared_ptr<Chunk> p_chunk = m_chunksToRender.pop().lock();
        if (!p_chunk || p_chunk->m_status1() != Status::MESH_GENERATED) break;
        p_chunk->setupBuffers();
    }

    // Call the callback for each chunk that is ready to be rendered
    std::lock_guard lock(m_chunksMutex);
    for (const auto& chunk : m_loadedChunks | std::views::values) {
        if (chunk && chunk->m_status1() == Status::BUFFERS_SETUP) {
            callback(chunk);
        }
    }
}

#endif //WORLD_H
