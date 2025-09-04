#ifndef MESHDATA_H
#define MESHDATA_H
#include <memory>
#include <unordered_map>

#include "ChunkPosition.h"
#include "PendingBlock.h"
#include "../../utils/ThreadSafeQueue.h"

struct MeshingResult {
    ChunkPosition position;
    std::vector<BlockVertex> opaqueVertices;
    std::vector<BlockVertex> waterVertices;
    bool hasOpaqueFaces = false;
    bool hasWaterFaces = false;
    bool needInstanceUpdate = false;
    bool needIndirectRendererUpdate = false;
};

template<typename MeshType>
class MeshManager {
public:
    std::unordered_map<ChunkPosition, std::shared_ptr<MeshType> > loadedMeshes;
    std::unordered_map<ChunkPosition, std::vector<PendingBlock> > pendingBlocks;
    std::mutex pendingBlocksMutex;
    unsigned int lastPendingBlockSize = 0;
    ThreadSafeQueue<ChunkPosition> meshesToGenerate;
    ThreadSafeQueue<std::shared_ptr<MeshType> > meshesToDelete;
    ThreadSafeQueue<MeshingResult> completedMeshes;
};


#endif //MESHDATA_H
