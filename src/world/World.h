#ifndef WORLD_H
#define WORLD_H

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
    std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> m_loadedChunks;
    ThreadSafeQueue<std::tuple<int, int, int>> m_chunksToGenerate;
    ThreadSafeQueue<std::shared_ptr<Chunk>> m_chunksToDelete;
    ThreadSafeQueue<std::shared_ptr<Chunk>> m_chunksToRender;
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
    const int maxChunksPerFrame = static_cast<int>(0.3 * Renderer::m_renderDistance + 0.6 * static_cast<float>(m_threadPool.m_num_threads()));
    // Remove chunks that are no longer needed
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (!m_chunksToDelete.empty()) m_chunksToDelete.pop();
    }

    // Process chunks that are within the render distance
    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksToGenerate.empty()) break;

        std::tuple<int, int, int> key = m_chunksToGenerate.pop();
        m_threadPool.enqueue([this, key] {
            const auto p_chunk = std::make_shared<Chunk>(std::get<0>(key), std::get<1>(key), std::get<2>(key));
            p_chunk->generateVoxel(m_terrainHeightGenerator, m_caveGenerator);
            p_chunk->generateMesh();
            if (!p_chunk->hasVisibleFaces()) {
                m_chunksToDelete.push(p_chunk);
                return;
            }
            m_chunksToRender.push(p_chunk);
        });
    }

    for (int i = 0; i < maxChunksPerFrame; ++i) {
        if (m_chunksToRender.empty()) break;
        std::shared_ptr<Chunk> p_chunk = m_chunksToRender.pop();
        std::tuple<int, int, int> key = std::make_tuple(p_chunk->m_x1(), p_chunk->m_y1(), p_chunk->m_z1());
        m_loadedChunks.try_emplace(key, p_chunk);
    }

    // Setup buffers for loaded chunks, callback is called for each chunk that has buffers set up
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
