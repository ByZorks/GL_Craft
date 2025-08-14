#ifndef WORLD_H
#define WORLD_H

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

struct Offset {
    int x, z, maxY;
};

class World {
private:
    ThreadPool m_threadPool;

    MeshData<Chunk> m_chunksData;
    std::vector<ChunkPosition> m_tempKeysToProcess;
    std::vector<Offset> m_renderDistanceOffsets;

    InstanceRenderer m_grassRenderer;
    InstanceRenderer m_poppyRenderer;
    InstanceRenderer m_cornflowerRenderer;
    InstanceRenderer m_alliumRenderer;

    std::mutex m_heightMapMutex;

    std::vector<std::shared_ptr<Chunk>> m_displayedNormalMeshes;
    std::vector<std::shared_ptr<Chunk>> m_displayedTransparentMeshes;
    std::vector<std::shared_ptr<Chunk>> m_displayedWaterMeshes;

public:
    World();

    void updateChunks(const Camera &camera);
    void drawChunks(const Camera &camera, const Frustum &frustum, Shader &shader, unsigned int &visibleChunksCount, unsigned int &drawCalls);
    void drawTransparentChunks(const Camera &camera, const Frustum &frustum,Shader &shader, unsigned int &drawCalls) const;
    void drawWater(const Camera &camera, const Frustum &frustum,Shader &shader, unsigned int &drawCalls) const;
    void drawInstances(unsigned int &drawCalls) const;
    void addPendingBlocks(const std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &blockData);
    void updateRenderDistance(Shader &postProcessingShader);

    static int getHeight(int worldX, int worldZ);
    static bool isCave(int worldX, int worldY, int worldZ, int columnHeight);

    [[nodiscard]] const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>> & m_loaded_chunks() const;
    static FastNoiseLite& getSurfaceFeaturesNoise();


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
