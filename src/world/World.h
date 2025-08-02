#ifndef WORLD_H
#define WORLD_H

#include <ranges>
#include <unordered_map>
#include <unordered_set>

#include "Chunk.h"

#include "FastNoiseLite.h"
#include "../gl/Shader.h"
#include "../math/Frustum.h"
#include "../render/ThreadPool.h"
#include "../utils/CustomHash.h"
#include "../utils/ThreadSafeQueue.h"
#include "vegetations/GrassInstanceRenderer.h"
#include "vegetations/Vegetation.h"

class Camera;

class World {
private:
    ThreadPool m_threadPool;

    std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> m_loadedChunks;
    ThreadSafeQueue<std::tuple<int, int, int>> m_chunksToGenerate;
    ThreadSafeQueue<std::shared_ptr<Chunk>> m_chunksToDelete;
    ThreadSafeQueue<std::shared_ptr<Chunk>> m_chunksToRender;

    std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Vegetation>> m_loadedVegetations;
    ThreadSafeQueue<std::tuple<int, int, int>> m_vegetationsToGenerate;
    ThreadSafeQueue<std::shared_ptr<Vegetation>> m_vegetationsToDelete;
    ThreadSafeQueue<std::shared_ptr<Vegetation>> m_vegetationsToRender;

    std::unordered_set<glm::vec3> m_loadedGrass;
    mutable std::mutex m_grassInstancesMutex;
    GrassInstanceRenderer m_grassRenderer;

    std::unordered_map<std::pair<int, int>, int> m_heightMap;
    mutable std::mutex m_heightMapMutex;

    std::vector<std::weak_ptr<Mesh>> m_displayedNormalMeshes;
    std::vector<std::weak_ptr<Mesh>> m_displayedBillboardsMeshes;

    FastNoiseLite m_terrainHeightGenerator;
    FastNoiseLite m_surfaceVegetationGenerator;
    FastNoiseLite m_caveGenerator;

public:
    World();
    ~World();

    void updateChunks(const Camera &camera, float renderDistanceInBlocks);
    void drawChunks(const Camera &camera, const Frustum &frustum, Shader &shader, unsigned int &visibleChunksCount);
    void drawVegetations(const Camera &camera, const Frustum &frustum, Shader &shader, unsigned int &visibleVegetationsCount);
    void drawInstances(const Camera &camera, const Frustum &frustum, unsigned int &visibleVegetationsCount);

    int getHeight(int worldX, int worldZ);
    bool isCave(int worldX, int worldY, int worldZ) const;

    [[nodiscard]] const FastNoiseLite & m_noise_generator() const;
    [[nodiscard]] const FastNoiseLite & m_surface_vegetation_generator() const;
    [[nodiscard]] const std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> & m_loaded_chunks() const;
    [[nodiscard]] const std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Vegetation>> &m_loaded_vegetations() const;

private:
    void processChunks();
    void processVegetations();

    void generateDataForEachChunks(float renderDistanceInBlocks, int cameraWorldX, int cameraWorldY, int cameraWorldZ);
    void generateVegetationsForEachChunks(const std::shared_ptr<Chunk>& chunk);
    void unloadDistantMeshes(const glm::vec3 &cameraChunkPos, float renderDistance);
};

#endif //WORLD_H
