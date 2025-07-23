#ifndef WORLD_H
#define WORLD_H

#include <iostream>
#include <mutex>
#include <ranges>
#include <unordered_map>

#include "Chunk.h"

#include "FastNoiseLite.h"
#include "../render/ThreadPool.h"
#include "../utils/ThreadSafeQueue.h"

class Camera;

class World {
private:
    std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> m_loadedChunks;
    mutable std::mutex m_chunksMutex;
    FastNoiseLite m_noiseGenerator;
    ThreadPool m_threadPool;
    ThreadSafeQueue<std::shared_ptr<Chunk>> m_chunksToRender;

public:
    World();
    ~World();

    std::shared_ptr<Chunk> getChunk(int chunkBaseX, int chunkBaseY, int chunkBaseZ) const;
    void updateChunks(Camera &camera, float renderDistanceInBlocks = 8.0f * static_cast<float>(Chunk::m_size1()));
    template<typename Callback>
    void forEachRenderableChunk(Callback&& callback);

    [[nodiscard]] const FastNoiseLite & m_noise_generator() const;
    [[nodiscard]] const std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> & m_loaded_chunks() const;

private:
    void unloadDistantChunks(const glm::vec3 &cameraChunkPos, int renderDistance);
};

template<typename Callback>
void World::forEachRenderableChunk(Callback &&callback) {
    // Process chunks that are ready to be rendered
    std::shared_ptr<Chunk> chunkToSetup;
    constexpr int maxToProcessPerFrame = 10;
    for (int i = 0; i < maxToProcessPerFrame && ((chunkToSetup = m_chunksToRender.pop())); ++i) {
        if (chunkToSetup && chunkToSetup->m_status1() == Status::MESH_GENERATED) {
            chunkToSetup->setupBuffers();
        }
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
