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

    std::unordered_map<std::pair<int, int>, int> m_heightMap;
    mutable std::mutex m_heightMapMutex;

    std::vector<std::weak_ptr<Mesh>> m_displayedMeshes;

    FastNoiseLite m_terrainHeightGenerator;
    FastNoiseLite m_surfaceVegetationGenerator;
    FastNoiseLite m_caveGenerator;

public:
    World();
    ~World();

    void updateChunks(Camera &camera, float renderDistanceInBlocks);
    void draw(const Camera &camera, const Frustum &frustum, Shader &shader, unsigned int &visibleChunksCount);

    int getHeight(int worldX, int worldZ);
    bool isCave(int worldX, int worldY, int worldZ) const;

    [[nodiscard]] const FastNoiseLite & m_noise_generator() const;
    [[nodiscard]] const FastNoiseLite & m_surface_vegetation_generator() const;
    [[nodiscard]] const std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> & m_loaded_chunks() const;

private:
    void generateDataForEachChunks(float renderDistanceInBlocks, int cameraWorldX, int cameraWorldY, int cameraWorldZ);
    void unloadDistantChunks(const glm::vec3 &cameraChunkPos, float renderDistance);
    void processChunks();
};

#endif //WORLD_H
