#ifndef GL_CRAFT_INDIRECTRENDERER_H
#define GL_CRAFT_INDIRECTRENDERER_H
#include <memory>
#include <vector>

#include "../gl/IndirectBuffer.h"
#include "../gl/StorageBuffer.h"
#include "../world/Chunk.h"

class IndirectRenderer {
private:
    // Structs
    struct DrawArraysIndirectCommand {
        unsigned int count;
        unsigned int instanceCount;
        unsigned int first;
        unsigned int baseInstance;
    };

    struct GPUSlot {
        bool isUsed = false;
        unsigned int vertexCount = 0;
        unsigned int vertexOffset = 0;
    };

    // Opaque
    IndirectBuffer m_opaqueIBO;
    StorageBuffer m_opaqueSSBO;
    StorageBuffer m_opaqueOffsetsSSBO;
    size_t m_opaqueHighestSlotUsed = 0;
    std::vector<GPUSlot> m_opaqueGPUSlots;

    // Transparent
    IndirectBuffer m_transparentIBO;
    StorageBuffer m_transparentSSBO;
    StorageBuffer m_transparentOffsetsSSBO;
    size_t m_transparentHighestSlotUsed = 0;
    std::vector<GPUSlot> m_transparentGPUSlots;

    // Water
    IndirectBuffer m_waterIBO;
    StorageBuffer m_waterSSBO;
    StorageBuffer m_waterOffsetsSSBO;
    size_t m_waterHighestSlotUsed = 0;
    std::vector<GPUSlot> m_waterGPUSlots;

    size_t m_maxVerticesPerMesh = Chunk::SIZE * Chunk::SIZE * Chunk::SIZE * 6; // Using vertex pulling, 1 vertex per block face

public:
    IndirectRenderer();

    void addChunk(const std::shared_ptr<Chunk> &chunk);
    void removeChunk(const std::shared_ptr<Chunk> &chunk);
    void updateChunk(const std::shared_ptr<Chunk> &chunk);

    void drawOpaque() const;
    void drawTransparent() const;
    void drawWater() const;

private:
    void addOpaqueChunk(const std::shared_ptr<Chunk> &chunk);
    void addTransparentChunk(const std::shared_ptr<Chunk> &chunk);
    void addWaterChunk(const std::shared_ptr<Chunk> &chunk);

    void removeOpaqueChunk(const std::shared_ptr<Chunk> &chunk);
    void removeTransparentChunk(const std::shared_ptr<Chunk> &chunk);
    void removeWaterChunk(const std::shared_ptr<Chunk> &chunk);

    void updateOpaqueChunk(const std::shared_ptr<Chunk> &chunk);
    void updateTransparentChunk(const std::shared_ptr<Chunk> &chunk);
    void updateWaterChunk(const std::shared_ptr<Chunk> &chunk);
};

#endif //GL_CRAFT_INDIRECTRENDERER_H