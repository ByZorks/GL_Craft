#ifndef GL_CRAFT_INDIRECTRENDERER_H
#define GL_CRAFT_INDIRECTRENDERER_H
#include <memory>
#include <vector>

#include "../gl/IndirectBuffer.h"
#include "../gl/StorageBuffer.h"
#include "../world/Chunk.h"

struct DrawArraysIndirectCommand {
    unsigned int count;
    unsigned int instanceCount;
    unsigned int first;
    unsigned int baseInstance;
};

class IndirectRenderer {
private:
    IndirectBuffer m_IBO;
    StorageBuffer m_SSBO;
    StorageBuffer m_offsetsSSBO;
    size_t m_opaqueCount = 0;
    size_t m_transparentCount = 0;
    size_t m_waterCount = 0;
    size_t m_opaqueFirstCmd = 0;
    size_t m_transparentFirstCmd = 0;
    size_t m_waterFirstCmd = 0;

public:
    IndirectRenderer();

    void createDrawCommands(const std::vector<std::shared_ptr<Chunk>> &opaqueMeshes, const std::vector<std::shared_ptr<Chunk>> &transparentMeshes, const std::vector<std::shared_ptr<Chunk>> &waterMeshes);
    void resetDrawCommands();
    void drawOpaque() const;
    void drawTransparent() const;
    void drawWater() const;
};

#endif //GL_CRAFT_INDIRECTRENDERER_H