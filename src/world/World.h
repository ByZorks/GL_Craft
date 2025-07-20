#ifndef WORLD_H
#define WORLD_H

#include <mutex>
#include <ranges>
#include <unordered_map>

#include "Chunk.h"
#include <vector>

#include "vec2.hpp"
#include "FastNoiseLite.h"

class Camera;

class World {
private:
    std::unordered_map<std::tuple<int, int, int>, Chunk*> m_loadedChunks;
    glm::vec3 m_lastCameraChunkPos = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };
    FastNoiseLite m_noiseGenerator;
    mutable std::mutex m_chunksMutex;
    static unsigned int s_numberOfThreads;

public:
    World();
    ~World();

    Chunk* getChunk(int chunkBaseX, int chunkBaseY, int chunkBaseZ) const;
    void updateChunks(const Camera &camera, float renderDistanceInBlocks = 8.0f * static_cast<float>(Chunk::m_size1()));
    const std::vector<Chunk*> & getChunksToRender();
    template<typename Callback>
    void forEachRenderableChunk(Callback&& callback) const;

    [[nodiscard]] const FastNoiseLite & m_noise_generator() const;

private:
    void unloadDistantChunks(const glm::vec3 &cameraChunkPos, int renderDistance);
    static void processChunk(Chunk *chunk, const World *world);
};

template<typename Callback>
void World::forEachRenderableChunk(Callback &&callback) const {
    std::lock_guard lock(m_chunksMutex);
    for (const auto& chunk : m_loadedChunks | std::views::values) {
        if (chunk->m_status1() == Status::BUFFERS_SETUP) {
            callback(chunk);
        }
    }
}

#endif //WORLD_H
