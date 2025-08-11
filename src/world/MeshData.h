#ifndef MESHDATA_H
#define MESHDATA_H
#include <memory>
#include <unordered_map>

#include "../utils/ThreadSafeQueue.h"

template<typename MeshType>
class MeshData {
public:
    std::unordered_map<ChunkPosition, std::shared_ptr<MeshType>> loadedMeshes;
    std::unordered_map<ChunkPosition, std::vector<std::tuple<int, int, int, BlockType>>> m_pendingBlocks;
    std::mutex m_pendingBlocksMutex;
    ThreadSafeQueue<ChunkPosition> meshesToGenerateVoxel;
    ThreadSafeQueue<std::shared_ptr<MeshType>> meshesToGeneratePendingBlocks;
    ThreadSafeQueue<std::shared_ptr<MeshType>> meshesToRemesh;
    ThreadSafeQueue<std::shared_ptr<MeshType>> meshesToDelete;
    ThreadSafeQueue<std::shared_ptr<MeshType>> meshesToRender;
};


#endif //MESHDATA_H
