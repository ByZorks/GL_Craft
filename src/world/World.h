#ifndef WORLD_H
#define WORLD_H

#include <mutex>
#include <unordered_map>

#include "Chunk.h"
#include <vector>

#include "vec2.hpp"
#include "FastNoiseLite.h"
#include "../utils/CustomHash.h"

class Camera;

class World {
private:
    std::unordered_map<std::tuple<int, int, int>, Chunk*> m_loadedChunks;
    glm::vec2 m_lastCameraChunkPos = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };
    FastNoiseLite m_noiseGenerator;
    mutable std::mutex m_chunksMutex;

public:
    World();
    ~World();

    Chunk* getChunk(int chunkBaseX, int chunkBaseY, int chunkBaseZ) const;
    void updateChunks(const Camera &camera, float renderDistanceInBlocks = 8.0f * static_cast<float>(Chunk::m_size1()));
    std::vector<Chunk*> getChunksToRender();

    [[nodiscard]] const FastNoiseLite & m_noise_generator() const;

private:
    void unloadDistantChunks(glm::vec3 cameraChunkPos, int renderDistance);
    static void processChunk(Chunk *chunk, const World *world);
};

#endif //WORLD_H
