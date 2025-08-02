#ifndef MESHDATA_H
#define MESHDATA_H
#include <memory>
#include <unordered_map>

#include "../utils/ThreadSafeQueue.h"
#include "../utils/CustomHash.h"

template<typename MeshType>
class MeshData {
public:
    std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<MeshType>> loadedMeshes;
    ThreadSafeQueue<std::tuple<int, int, int>> meshesToGenerate;
    ThreadSafeQueue<std::shared_ptr<MeshType>> meshesToDelete;
    ThreadSafeQueue<std::shared_ptr<MeshType>> meshesToRender;

    MeshData() = default;
};


#endif //MESHDATA_H
