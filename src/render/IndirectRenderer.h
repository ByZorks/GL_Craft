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
        bool useNextSlot = false;
    };

    struct MeshData {
        IndirectBuffer IBO;
        StorageBuffer verticesSSBO;
        StorageBuffer offsetsSSBO;
        size_t highestSlotUsed = 0;
        size_t count = 0;
        std::vector<GPUSlot> gpuSlots;
        std::queue<unsigned int> freeDrawIndices;
    };

    enum class MeshType {
        OPAQUE,
        TRANSPARENT,
        WATER
    };

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
    void add(MeshData &meshData, MeshType meshType, const std::shared_ptr<Chunk> &chunk) const;
    static void remove(MeshData &meshData, MeshType meshType, const std::shared_ptr<Chunk> &chunk);
    void update(MeshData &meshData, MeshType meshType, const std::shared_ptr<Chunk> &chunk) const;
};

#endif //GL_CRAFT_INDIRECTRENDERER_H