#ifndef WORLD_H
#define WORLD_H

#include <ranges>
#include <unordered_map>

#include "Chunk.h"

#include "FastNoiseLite.h"
#include "MeshData.h"
#include "../gl/Shader.h"
#include "../math/Frustum.h"
#include "../render/InstanceRenderer.h"
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

    InstanceRenderer m_grassRenderer;
    InstanceRenderer m_poppyRenderer;
    InstanceRenderer m_cornflowerRenderer;
    InstanceRenderer m_alliumRenderer;

    std::unordered_map<std::pair<int, int>, ChunkHeightmap> m_heightMapByChunk;
    std::mutex m_heightMapMutex;

    std::vector<std::shared_ptr<Chunk>> m_displayedNormalMeshes;
    std::vector<std::shared_ptr<Chunk>> m_displayedTransparentMeshes;
    std::vector<std::shared_ptr<Chunk>> m_displayedWaterMeshes;

    FastNoiseLite m_terrainHeightGenerator;
    FastNoiseLite m_surfaceFeaturesNoise;
    FastNoiseLite m_caveGenerator;

public:
    World();

    void updateChunks(const Camera &camera);
    void drawChunks(const Camera &camera, const Frustum &frustum, Shader &shader, unsigned int &visibleChunksCount, unsigned int &drawCalls);
    void drawTransparentChunks(Shader &shader, unsigned int &drawCalls) const;
    void drawWater(Shader &shader, unsigned int &drawCalls) const;
    void drawInstances(unsigned int &drawCalls) const;

    void addPendingBlocks(const std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &blockData);
    int getHeight(int worldX, int worldZ);
    bool isCave(int worldX, int worldY, int worldZ, int columnHeight) const;

    [[nodiscard]] const FastNoiseLite & m_noise_generator() const;
    [[nodiscard]] const FastNoiseLite & m_surface_features_noise() const;
    [[nodiscard]] const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>> & m_loaded_chunks() const;

private:
    void processChunks();
    void generateDataForEachChunks(int cameraWorldX, int cameraWorldY, int cameraWorldZ);
    void unloadDistantMeshes(const glm::vec3 &cameraChunkPos);
};

#endif //WORLD_H
