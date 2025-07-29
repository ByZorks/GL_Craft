#ifndef WORLD_H
#define WORLD_H

#include <ranges>
#include <unordered_map>

#include "Chunk.h"

#include "FastNoiseLite.h"
#include "../gl/Shader.h"
#include "../math/Frustum.h"
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
    std::vector<std::weak_ptr<Mesh>> m_opaqueMeshes;
    std::vector<std::weak_ptr<Mesh>> m_transparentMeshes;
    FastNoiseLite m_terrainHeightGenerator;
    FastNoiseLite m_surfaceVegetationGenerator;
    FastNoiseLite m_caveGenerator;

public:
    World();
    ~World();

    void updateChunks(Camera &camera, float renderDistanceInBlocks);
    void draw(Camera &camera, const Frustum &frustum, Shader &shader, unsigned int &visibleChunksCount);

    [[nodiscard]] const FastNoiseLite & m_noise_generator() const;
    [[nodiscard]] const std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> & m_loaded_chunks() const;

private:
    void generateDataForEachChunks(float renderDistanceInBlocks, int cameraWorldX, int cameraWorldY, int cameraWorldZ);
    void unloadDistantChunks(const glm::vec3 &cameraChunkPos, float renderDistance);
    void processChunks();
};

#endif //WORLD_H
