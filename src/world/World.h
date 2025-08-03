#ifndef WORLD_H
#define WORLD_H

#include <ranges>
#include <unordered_map>

#include "Chunk.h"

#include "FastNoiseLite.h"
#include "MeshData.h"
#include "../gl/Shader.h"
#include "../math/Frustum.h"
#include "../render/InstanceRendererData.h"
#include "../render/ThreadPool.h"
#include "../utils/CustomHash.h"
#include "vegetations/grass/GrassInstanceRenderer.h"
#include "vegetations/Vegetation.h"
#include "vegetations/flowers/AlliumInstanceRenderer.h"
#include "vegetations/flowers/CornflowerInstanceRenderer.h"
#include "vegetations/flowers/PoppyInstanceRenderer.h"

class Camera;

class World {
private:
    ThreadPool m_threadPool;

    MeshData<Chunk> m_chunksData;
    MeshData<Vegetation> m_vegetationsData;

    InstanceRendererData<GrassInstanceRenderer> m_grassData;
    InstanceRendererData<PoppyInstanceRenderer> m_poppyData;
    InstanceRendererData<CornflowerInstanceRenderer> m_cornflowerData;
    InstanceRendererData<AlliumInstanceRenderer> m_alliumData;

    std::unordered_map<std::pair<int, int>, int> m_heightMap;
    mutable std::mutex m_heightMapMutex;

    std::vector<std::weak_ptr<Mesh>> m_displayedNormalMeshes;
    std::vector<std::weak_ptr<Mesh>> m_displayedBillboardsMeshes;

    FastNoiseLite m_terrainHeightGenerator;
    FastNoiseLite m_surfaceVegetationGenerator;
    FastNoiseLite m_caveGenerator;

public:
    World();

    void updateChunks(const Camera &camera);
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
    void generateDataForEachChunks(int cameraWorldX, int cameraWorldY, int cameraWorldZ);
    void generateVegetationsForEachChunks(const std::shared_ptr<Chunk>& chunk);
    void unloadDistantMeshes(const glm::vec3 &cameraChunkPos);
};

#endif //WORLD_H
