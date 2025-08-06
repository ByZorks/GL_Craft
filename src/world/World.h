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

class Camera;

struct ChunkHeightmap {
    std::array<int, Chunk::SIZE * Chunk::SIZE> heights;

    [[nodiscard]] int getHeight(const int localX, const int localZ) const {
        return heights[localX + localZ * Chunk::SIZE];
    }
};


class World {
private:
    ThreadPool m_threadPool;

    MeshData<Chunk> m_chunksData;
    MeshData<Mesh> m_vegetationsData;

    InstanceRendererData m_grassData;
    InstanceRendererData m_poppyData;
    InstanceRendererData m_cornflowerData;
    InstanceRendererData m_alliumData;

    std::unordered_map<std::pair<int, int>, ChunkHeightmap> m_heightMapByChunk;
    mutable std::mutex m_heightMapMutex;

    std::vector<std::weak_ptr<Mesh>> m_displayedNormalMeshes;
    std::vector<std::weak_ptr<Mesh>> m_displayedTransparentMeshes;

    FastNoiseLite m_terrainHeightGenerator;
    FastNoiseLite m_surfaceVegetationGenerator;
    FastNoiseLite m_caveGenerator;

public:
    World();

    void updateChunks(const Camera &camera);
    void drawChunks(const Camera &camera, const Frustum &frustum, Shader &shader, unsigned int &visibleChunksCount, unsigned int &drawCalls);
    void drawWater(Shader &waterShader, unsigned int &drawCalls) const;
    void drawVegetations(const Camera &camera, const Frustum &frustum, Shader &shader, unsigned int &visibleVegetationsCount, unsigned int &drawCalls);
    void drawInstances(const Camera &camera, const Frustum &frustum, unsigned int &visibleVegetationsCount, unsigned int &drawCalls);

    int getHeight(int worldX, int worldZ);
    bool isCave(int worldX, int worldY, int worldZ, int columnHeight) const;

    [[nodiscard]] const FastNoiseLite & m_noise_generator() const;
    [[nodiscard]] const FastNoiseLite & m_surface_vegetation_generator() const;
    [[nodiscard]] const std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> & m_loaded_chunks() const;
    [[nodiscard]] const std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Mesh>> &m_loaded_vegetations() const;

private:
    void processChunks();
    void processVegetations();
    void generateDataForEachChunks(int cameraWorldX, int cameraWorldY, int cameraWorldZ);
    void generateVegetationsForEachChunks(const std::shared_ptr<Chunk>& chunk);
    void unloadDistantMeshes(const glm::vec3 &cameraChunkPos);
};

#endif //WORLD_H
