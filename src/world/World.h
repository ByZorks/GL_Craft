#ifndef WORLD_H
#define WORLD_H

#include <unordered_map>

#include "Chunk.h"

#include "MeshManager.h"
#include "../gl/Shader.h"
#include "../math/Frustum.h"
#include "../math/Raycast.h"
#include "../render/InstanceRenderer.h"
#include "../utils/ThreadPool.h"
#include "fastNoiseLite/FastNoiseLite.h"

class Camera;

struct Offset {
    int x, z, maxY;
};

class World {
private:
    ThreadPool m_threadPool;

    MeshManager<Chunk> m_chunksData;
    std::vector<ChunkPosition> m_tempKeysToProcess;
    std::vector<Offset> m_renderDistanceOffsets;

    InstanceRenderer m_grassRenderer;
    InstanceRenderer m_poppyRenderer;
    InstanceRenderer m_cornflowerRenderer;
    InstanceRenderer m_alliumRenderer;
    bool m_renderDistanceChanged = false;
    bool m_instancesChanged = false;

    std::mutex m_heightMapMutex;

    std::vector<std::shared_ptr<Chunk>> m_displayedNormalMeshes;
    std::vector<std::shared_ptr<Chunk>> m_displayedTransparentMeshes;
    std::vector<std::shared_ptr<Chunk>> m_displayedWaterMeshes;

public:
    World();

    void updateChunks(const Camera &camera);
    void drawChunks(const Camera &camera, const Frustum &frustum, Shader &shader, unsigned int &visibleChunksCount, unsigned int &drawCalls);
    void drawTransparentChunks(const Frustum &frustum,Shader &shader, unsigned int &drawCalls) const;
    void drawWater(const Frustum &frustum,Shader &shader, const glm::vec3 &cameraPos, unsigned int &drawCalls) const;
    void drawInstances(unsigned int &drawCalls) const;
    void addPendingBlocks(const std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &blockData);
    void updateRenderDistance(Shader &postProcessingShader, const Camera &camera);
    void deleteBlockAndUpdateNeighbors(const RaycastResult &hit);
    void placeBlockAndUpdateNeighbors(const RaycastResult &hit, BlockType blockToPlace);

    static int getHeight(int worldX, int worldZ);
    static bool isCave(int worldX, int worldY, int worldZ, int columnHeight);
    std::shared_ptr<Chunk> getChunk(int x, int y, int z) const;

    [[nodiscard]] ThreadPool & getThreadPool();
    [[nodiscard]] const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>> & getLoadedChunks() const;
    [[nodiscard]] ThreadSafeQueue<std::shared_ptr<Chunk>> & getMeshesToUpdate();
    static FastNoiseLite& getSurfaceFeaturesNoise();

    void setInstancesChanged(bool m_instances_changed);

private:
    void processChunks();
    void generateChunksPositions(int cameraWorldX, int cameraWorldY, int cameraWorldZ);
    void unloadDistantMeshes(const glm::vec3 &cameraChunkPos);
    static FastNoiseLite makeTerrainNoise();
    static FastNoiseLite makeSurfaceFeaturesNoise();
    static FastNoiseLite makeCaveNoise();
    static FastNoiseLite& getTerrainNoise();
    static FastNoiseLite& getCaveNoise();
};

#endif //WORLD_H
