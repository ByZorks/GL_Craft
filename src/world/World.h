#ifndef WORLD_H
#define WORLD_H

#include <unordered_map>

#include "Chunk.h"

#include "FastNoiseLite.h"
#include "MeshManager.h"
#include "../gl/Shader.h"
#include "../math/Frustum.h"
#include "../math/Raycast.h"
#include "../render/IndirectRenderer.h"
#include "../render/InstanceRenderer.h"
#include "../utils/ThreadPool.h"

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

    IndirectRenderer m_indirectRenderer;
    bool m_indirectRendererNeedsUpdate = false;

    InstanceRenderer m_grassRenderer;
    InstanceRenderer m_poppyRenderer;
    InstanceRenderer m_cornflowerRenderer;
    InstanceRenderer m_alliumRenderer;
    bool m_renderDistanceChanged = false;
    bool m_needInstanceUpdate = false;

    std::vector<std::shared_ptr<Chunk>> m_displayedNormalMeshes;
    std::vector<std::shared_ptr<Chunk>> m_displayedTransparentMeshes;
    std::vector<std::shared_ptr<Chunk>> m_displayedWaterMeshes;

public:
    World();

    void updateChunks(const Camera &camera, const Frustum &frustum);
    void draw(const Shader &blockShader, const Shader &waterShader, const Shader &instancesShader, unsigned int &drawCmd) const;
    void addPendingBlocks(const std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &blockData);
    void updateRenderDistance(Shader &postProcessingShader, const Camera &camera, const Frustum &frustum);
    void deleteBlockAndUpdateNeighbors(const RaycastResult &hit);
    void placeBlockAndUpdateNeighbors(const RaycastResult &hit, BlockType blockToPlace);

    static int getHeight(int worldX, int worldZ);
    static bool isCave(int worldX, int worldY, int worldZ, int columnHeight);
    std::shared_ptr<Chunk> getChunk(int x, int y, int z) const;

    [[nodiscard]] ThreadPool & getThreadPool();
    [[nodiscard]] const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>> & getLoadedChunks() const;
    static FastNoiseLite& getSurfaceFeaturesNoise();
    [[nodiscard]] unsigned int getVisibleChunksCount() const;

private:
    void processChunksQueues();
    void sortChunks(const Frustum &frustum, const Camera &camera);
    void drawInstances(unsigned int &drawCmd) const;
    void generateChunksPositions(int cameraWorldX, int cameraWorldY, int cameraWorldZ);
    void unloadDistantMeshes(const glm::vec3 &cameraChunkPos);
    static FastNoiseLite makeTerrainNoise();
    static FastNoiseLite makeSurfaceFeaturesNoise();
    static FastNoiseLite makeCaveNoise();
    static FastNoiseLite& getTerrainNoise();
    static FastNoiseLite& getCaveNoise();
};

#endif //WORLD_H
