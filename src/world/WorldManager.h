#ifndef GL_CRAFT_WORLDMANAGER_H
#define GL_CRAFT_WORLDMANAGER_H
#include "../gl/Shader.h"
#include "../math/Raycast.h"
#include "../render/Camera.h"
#include "../render/IndirectRenderer.h"
#include "../utils/ThreadPool.h"
#include "chunk/Chunk.h"
#include "chunk/MeshManager.h"

class WorldManager {
public:
    WorldManager();

    void updateChunks(const Camera &camera, IndirectRenderer &renderer);
    void updateRenderDistance(Shader &postProcessingShader, const Camera &camera, IndirectRenderer &renderer);
    void addPendingBlocks(std::unordered_map<ChunkPosition, std::list<PendingBlock>> &blockData);
    void addPendingLights(std::unordered_map<ChunkPosition, std::list<PendingLight>> &lightData);
    void deleteBlockAndUpdateNeighbors(const RaycastResult &hit);
    void placeBlockAndUpdateNeighbors(const RaycastResult &hit, Block::BlockType blockToPlace);

    [[nodiscard]] const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk>> & getLoadedChunks() const;
    [[nodiscard]] bool getNeedInstanceUpdate() const;

private:
    void processChunksQueues(IndirectRenderer &renderer);
    void deleteQueuedChunks(IndirectRenderer &renderer, unsigned int maxProcessPerFrame);
    void generateQueuedChunks(unsigned int maxProcessPerFrame);
    void generateQueuedPendingBlocks(unsigned int maxProcessPerFrame);
    void generateQueuedPendingLights(unsigned int maxProcessPerFrame);
    void updateQueuedChunks(IndirectRenderer &renderer, unsigned int maxProcessPerFrame);
    void generateChunksPositions(int cameraWorldX, int cameraWorldY, int cameraWorldZ);
    void getDistantChunks(const glm::vec3 &cameraChunkPos);
    std::shared_ptr<Chunk> getChunk(const ChunkPosition &position) const;
    void emitNeighborsBorderLights(const std::shared_ptr<Chunk> &chunk);

    void updateAdjacentChunksAfterDeletion(const std::shared_ptr<Chunk> &chunk, const std::array<int, 3> &blockLocalPos,
                                                     Block::BlockType blockType, std::unordered_set<ChunkPosition> &outProcessedChunks);
    void processAdjacentChunkOnDeletion(const ChunkPosition &neighborPos, const std::array<int, 3> &blockLocalPos,
                                                  const std::array<int, 3> &neighborOffset, Block::BlockType blockType,
                                                  bool isInstance, bool isLightEmitter, std::unordered_set<ChunkPosition> &outProcessedChunks);
    void propagateLightRemoval(const std::shared_ptr<Chunk> &chunk, const std::array<int, 3> &blockLocalPos,
                                         const std::unordered_set<ChunkPosition> &outProcessedChunks);
    void asyncDeleteBlock(const RaycastResult &hit);

    void asyncPlaceBlock(const RaycastResult &hit, Block::BlockType blockToPlace);
    std::pair<std::shared_ptr<Chunk>, std::array<int, 3>> determineTargetChunkAndPosition(const RaycastResult &hit) const;
    void updateAdjacentChunksAfterPlacement(const std::shared_ptr<Chunk> &chunk, const std::array<int, 3> &blockLocalPos,
                                                      Block::BlockType blockType, std::unordered_set<ChunkPosition> &outProcessedChunks);
    void processAdjacentChunkOnPlacement(const ChunkPosition &neighborPos, const std::array<int, 3> &blockLocalPos,
                                                   const std::array<int, 3> &neighborOffset, Block::BlockType blockType,
                                                   bool isInstance, bool isLightEmitter,
                                                   std::unordered_set<ChunkPosition> &outProcessedChunks);
    void propagateLightAddition(const std::shared_ptr<Chunk> &chunk, const std::array<int, 3> &blockLocalPos,
                                          const std::unordered_set<ChunkPosition> &processedChunks);

private:
    struct Offset {
        int x, z, maxY;
    };

    ThreadPool m_threadPool;
    MeshManager<Chunk> m_chunksData;
    std::vector<Offset> m_renderDistanceOffsets;
    bool m_renderDistanceChanged = false;
    std::atomic_bool m_needInstanceUpdate = false;

};

#endif //GL_CRAFT_WORLDMANAGER_H
