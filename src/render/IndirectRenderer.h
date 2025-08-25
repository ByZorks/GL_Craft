#ifndef GL_CRAFT_INDIRECTRENDERER_H
#define GL_CRAFT_INDIRECTRENDERER_H
#include <memory>
#include <queue>
#include <vector>

#include "../gl/IndirectBuffer.h"
#include "../gl/StorageBuffer.h"
#include "../world/Chunk.h"

class IndirectRenderer {
private:
    // Structs and enums
    struct DrawArraysIndirectCommand {
        unsigned int count = 0;
        unsigned int instanceCount = 0;
        unsigned int first = 0;
        unsigned int baseInstance = 0;
    };

    struct GPUSlot { // Size: 4 KiB = 1000 BlockVertex (1 unsigned int)
        bool isUsed = false;
        uint8_t numberOfSlotsUsed = 0; // 0 means it is not the first slot of a multi-slot allocation
    };

    struct MeshData {
        IndirectBuffer IBO;
        StorageBuffer offsetsSSBO;
        size_t count = 0;
        std::queue<unsigned int> freeDrawIndices;
    };

    enum class MeshType {
        OPAQUE,
        TRANSPARENT,
        WATER
    };

    StorageBuffer m_verticesSSBO;
    size_t m_highestSlotUsed = 0;
    std::vector<GPUSlot> m_gpuSlots;
    MeshData m_opaqueData;
    MeshData m_transparentData;
    MeshData m_waterData;

    unsigned int m_vertexPerSlot = 1000;

public:
    IndirectRenderer();

    void addChunk(const std::shared_ptr<Chunk> &chunk);
    void removeChunk(const std::shared_ptr<Chunk> &chunk);
    void updateChunk(const std::shared_ptr<Chunk> &chunk);

    void drawOpaque() const;
    void drawTransparent() const;
    void drawWater() const;

private:
    void add(MeshData &meshData, MeshType meshType, const std::shared_ptr<Chunk> &chunk);
    void remove(MeshData &meshData, MeshType meshType, const std::shared_ptr<Chunk> &chunk);
    void update(MeshData &meshData, MeshType meshType, const std::shared_ptr<Chunk> &chunk);
};

#endif //GL_CRAFT_INDIRECTRENDERER_H