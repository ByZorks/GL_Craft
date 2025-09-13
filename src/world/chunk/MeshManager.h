#ifndef MESHDATA_H
#define MESHDATA_H
#include <list>
#include <memory>
#include <unordered_map>

#include "ChunkPosition.h"
#include "PendingBlock.h"
#include "PendingLight.h"
#include "../../utils/ThreadSafeQueue.h"

struct MeshingResult {
    std::vector<Block::BlockVertex> opaqueVertices;
    std::vector<Block::BlockVertex> waterVertices;
    ChunkPosition position {0, 0, 0};
    bool hasOpaqueFaces = false;
    bool hasWaterFaces = false;
    bool needInstanceUpdate = false;
    bool needIndirectRendererUpdate = false;
};

template<typename MeshType>
class MeshManager {
public:
    std::unordered_map<ChunkPosition, std::shared_ptr<MeshType> > loadedMeshes;

    std::unordered_map<ChunkPosition, std::list<PendingBlock> > pendingBlocks;
    std::mutex pendingBlocksMutex;

    std::unordered_map<ChunkPosition, std::list<PendingLight> > pendingLights;
    std::mutex pendingLightsMutex;

    ThreadSafeQueue<ChunkPosition> meshesToGenerate;
    ThreadSafeQueue<std::shared_ptr<MeshType> > meshesToDelete;
    ThreadSafeQueue<MeshingResult> completedMeshes;
};

#endif //MESHDATA_H
