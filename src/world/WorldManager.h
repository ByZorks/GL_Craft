#ifndef GL_CRAFT_WORLDMANAGER_H
#define GL_CRAFT_WORLDMANAGER_H
#include "chunk/Chunk.h"
#include "chunk/MeshManager.h"
#include "../gl/Shader.h"
#include "../math/Raycast.h"
#include "../render/Camera.h"
#include "../render/IndirectRenderer.h"
#include "../utils/ThreadPool.h"

class WorldManager {
private:
    // Structs
    struct Offset {
        int x, z, maxY;
    };

    // Members
    ThreadPool m_threadPool;
    MeshManager<Chunk> m_chunksData;
    std::vector<ChunkPosition> m_tempKeysToProcess;
    std::vector<Offset> m_renderDistanceOffsets;
    bool m_renderDistanceChanged = false;
    std::atomic_bool m_needInstanceUpdate = false;

public:
    WorldManager();

    void updateChunks(const Camera &camera, IndirectRenderer &renderer);
    void updateRenderDistance(Shader &postProcessingShader, const Camera &camera, IndirectRenderer &renderer);
    void addPendingBlocks(const std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &blockData);
    void deleteBlockAndUpdateNeighbors(const RaycastResult &hit);
    void placeBlockAndUpdateNeighbors(const RaycastResult &hit, BlockType blockToPlace);

    [[nodiscard]] const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>> & getLoadedChunks() const;
    [[nodiscard]] bool getNeedInstanceUpdate() const;

private:
    void processChunksQueues(IndirectRenderer &renderer);
    void generateChunksPositions(int cameraWorldX, int cameraWorldY, int cameraWorldZ);
    void getDistantChunks(const glm::vec3 &cameraChunkPos);
    std::shared_ptr<Chunk> getChunk(int x, int y, int z) const;

};

#endif //GL_CRAFT_WORLDMANAGER_H
