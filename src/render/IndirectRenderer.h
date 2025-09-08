#ifndef GL_CRAFT_INDIRECTRENDERER_H
#define GL_CRAFT_INDIRECTRENDERER_H
#include <memory>
#include <queue>
#include <vector>

#include "../gl/IndirectBuffer.h"
#include "../gl/StorageBuffer.h"
#include "../world/chunk/Chunk.h"

class IndirectRenderer {
private:
    struct MeshData {
        std::queue<unsigned int> freeDrawIndices;
        StorageBuffer offsetsSSBO;
        IndirectBuffer IBO;
        size_t count = 0;
    };

    enum class MeshType : uint8_t {
        OPAQUE,
        WATER
    };

public:
    IndirectRenderer();

    void addChunk(const std::shared_ptr<Chunk> &chunk);
    void removeChunk(const std::shared_ptr<Chunk> &chunk);
    void updateChunk(const std::shared_ptr<Chunk> &chunk);
    void drawOpaque() const;
    void drawWater() const;

private:
    void add(MeshData &meshData, MeshType meshType, const std::shared_ptr<Chunk> &chunk);
    void remove(MeshData &meshData, MeshType meshType, const std::shared_ptr<Chunk> &chunk);
    void update(MeshData &meshData, MeshType meshType, const std::shared_ptr<Chunk> &chunk);

    void resizeVertexSSBOAndSlots(size_t ssboSize);

private:
    struct DrawArraysIndirectCommand {
        unsigned int count = 0;
        unsigned int instanceCount = 0;
        unsigned int first = 0;
        unsigned int baseInstance = 0;
    };

    struct GPUSlot {
        // Size: 4 KiB = 1000 BlockVertex (1 unsigned int)
        bool isUsed = false;
        uint8_t numberOfSlotsUsed = 0; // 0 means it is not the first slot of a multi-slot allocation
    };

    StorageBuffer m_verticesSSBO;
    Block::BlockVertex *m_mappedVertices = nullptr;
    size_t m_highestSlotUsed = 0;
    std::vector<GPUSlot> m_gpuSlots;
    MeshData m_opaqueData;
    MeshData m_waterData;

    unsigned int m_vertexPerSlot = 1000;
};

#endif //GL_CRAFT_INDIRECTRENDERER_H
