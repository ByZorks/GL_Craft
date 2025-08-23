#include "IndirectRenderer.h"

#include <iostream>

#include "Renderer.h"
#include "../world/Mesh.h"

IndirectRenderer::IndirectRenderer() {
    const auto renderDistanceInChunks = static_cast<size_t>(Renderer::s_renderDistance / Chunk::SIZE);
    const size_t chunksVisible = renderDistanceInChunks * renderDistanceInChunks * renderDistanceInChunks / 2; // Rough estimate
    const size_t IBOSize = sizeof(DrawArraysIndirectCommand) * chunksVisible;
    const size_t SSBOSize = sizeof(BlockVertex) * m_maxVerticesPerMesh * chunksVisible;
    const size_t offsetsSSBOSize = sizeof(std::array<int, 4>) * chunksVisible;
    const std::vector<DrawArraysIndirectCommand> emptyCommands(chunksVisible, {0, 0, 0, 0});

    // Opaque
    m_opaqueIBO.init(emptyCommands.data(), IBOSize);
    m_opaqueSSBO.init(nullptr, SSBOSize, 1);
    m_opaqueOffsetsSSBO.init(nullptr, offsetsSSBOSize, 2);
    m_opaqueGPUSlots.resize(chunksVisible);

    // Transparent
    m_transparentIBO.init(emptyCommands.data(), IBOSize);
    m_transparentSSBO.init(nullptr, SSBOSize, 1);
    m_transparentOffsetsSSBO.init(nullptr, offsetsSSBOSize, 2);
    m_transparentGPUSlots.resize(chunksVisible);

    // Water
    m_waterIBO.init(emptyCommands.data(), IBOSize);
    m_waterSSBO.init(nullptr, SSBOSize, 1);
    m_waterOffsetsSSBO.init(nullptr, offsetsSSBOSize, 2);
    m_waterGPUSlots.resize(chunksVisible);
}

void IndirectRenderer::addChunk(const std::shared_ptr<Chunk> &chunk) {
    if (chunk->getGPUSlotOpaque() == UINT_MAX && chunk->hasOpaqueFaces()) addOpaqueChunk(chunk);
    if (chunk->getGPUSlotTransparent() == UINT_MAX && chunk->hasTransparentFaces()) addTransparentChunk(chunk);
    if (chunk->getGPUSlotWater() == UINT_MAX && chunk->hasWaterFaces()) addWaterChunk(chunk);
}

void IndirectRenderer::removeChunk(const std::shared_ptr<Chunk> &chunk) {
    if (chunk->getGPUSlotOpaque() != UINT_MAX && chunk->hasOpaqueFaces()) removeOpaqueChunk(chunk);
    if (chunk->getGPUSlotTransparent() != UINT_MAX && chunk->hasTransparentFaces()) removeTransparentChunk(chunk);
    if (chunk->getGPUSlotWater() != UINT_MAX && chunk->hasWaterFaces()) removeWaterChunk(chunk);
}

void IndirectRenderer::updateChunk(const std::shared_ptr<Chunk> &chunk) {
    const bool hasOpaque = chunk->hasOpaqueFaces();
    const bool hasTransparent = chunk->hasTransparentFaces();
    const bool hasWater = chunk->hasWaterFaces();
    const unsigned int opaqueSlot = chunk->getGPUSlotOpaque();
    const unsigned int transparentSlot = chunk->getGPUSlotTransparent();
    const unsigned int waterSlot = chunk->getGPUSlotWater();

    // Chunk can have new type of faces, so we may need to add it to a new slot
    if (opaqueSlot != UINT_MAX && hasOpaque) {
        updateOpaqueChunk(chunk);
    } else if (opaqueSlot == UINT_MAX && hasOpaque) {
        addOpaqueChunk(chunk);
    }

    if (transparentSlot != UINT_MAX && hasTransparent) {
        updateTransparentChunk(chunk);
    } else if (transparentSlot == UINT_MAX && hasTransparent) {
        addTransparentChunk(chunk);
    }

    if (waterSlot != UINT_MAX && hasWater) {
        updateWaterChunk(chunk);
    } else if (waterSlot == UINT_MAX && hasWater) {
        addWaterChunk(chunk);
    }
}

void IndirectRenderer::drawOpaque() const {
    Renderer::drawMultiWithVertexPulling(m_opaqueIBO, m_opaqueSSBO, m_opaqueOffsetsSSBO, m_opaqueHighestSlotUsed, nullptr);
}

void IndirectRenderer::drawTransparent() const {
    Renderer::drawMultiWithVertexPulling(m_transparentIBO, m_transparentSSBO, m_transparentOffsetsSSBO, m_transparentHighestSlotUsed, nullptr);
}

void IndirectRenderer::drawWater() const {
    Renderer::drawMultiWithVertexPulling(m_waterIBO, m_waterSSBO, m_waterOffsetsSSBO, m_waterHighestSlotUsed, nullptr);
}

void IndirectRenderer::addOpaqueChunk(const std::shared_ptr<Chunk> &chunk) {
    // Find a free GPU slot
    unsigned int slotIndex = UINT_MAX;
    for (unsigned int i = 0; i < m_opaqueGPUSlots.size(); i++) {
        if (!m_opaqueGPUSlots[i].isUsed) {
            slotIndex = i;
            break;
        }
    }

    if (slotIndex == UINT_MAX) {
        std::cerr << "No free GPU slot available!\n";
        return;
    }

    const unsigned int vertexCount = chunk->getOpaqueVertexCount();
    auto &gpuSlot = m_opaqueGPUSlots[slotIndex];
    gpuSlot.isUsed = true;
    gpuSlot.vertexCount = vertexCount;
    gpuSlot.vertexOffset = slotIndex * m_maxVerticesPerMesh;
    chunk->setGPUSlotOpaque(slotIndex);

    DrawArraysIndirectCommand cmd{};
    cmd.count = vertexCount;
    cmd.instanceCount = 1;
    cmd.first = 0;
    cmd.baseInstance = 0;

    m_opaqueIBO.updateData(&cmd, sizeof(cmd), slotIndex * sizeof(DrawArraysIndirectCommand));

    const std::array offsets = { // 4 integers for x, y, z, and a padding value
        chunk->getX(),
        chunk->getY(),
        chunk->getZ(),
        0
    };
    m_opaqueOffsetsSSBO.updateData(offsets.data(), offsets.size() * sizeof(int), slotIndex * sizeof(std::array<int,4>));

    const auto &vertices = chunk->getOpaqueVertices();
    m_opaqueSSBO.updateData(vertices.data(), vertices.size() * sizeof(BlockVertex), gpuSlot.vertexOffset * sizeof(BlockVertex));

    m_opaqueHighestSlotUsed = std::max(m_opaqueHighestSlotUsed, static_cast<size_t>(slotIndex) + 1);
}

void IndirectRenderer::addTransparentChunk(const std::shared_ptr<Chunk> &chunk) {
    // Find a free GPU slot
    unsigned int slotIndex = UINT_MAX;
    for (unsigned int i = 0; i < m_transparentGPUSlots.size(); i++) {
        if (!m_transparentGPUSlots[i].isUsed) {
            slotIndex = i;
            break;
        }
    }

    if (slotIndex == UINT_MAX) {
        std::cerr << "No free GPU slot available!\n";
        return;
    }

    const unsigned int vertexCount = chunk->getTransparentVertexCount();
    auto &gpuSlot = m_transparentGPUSlots[slotIndex];
    gpuSlot.isUsed = true;
    gpuSlot.vertexCount = vertexCount;
    gpuSlot.vertexOffset = slotIndex * m_maxVerticesPerMesh;
    chunk->setGPUSlotTransparent(slotIndex);

    DrawArraysIndirectCommand cmd{};
    cmd.count = vertexCount;
    cmd.instanceCount = 1;
    cmd.first = 0;
    cmd.baseInstance = 0;

    m_transparentIBO.updateData(&cmd, sizeof(cmd), slotIndex * sizeof(DrawArraysIndirectCommand));

    const std::array offsets = { // 4 integers for x, y, z, and a padding value
        chunk->getX(),
        chunk->getY(),
        chunk->getZ(),
        0
    };
    m_transparentOffsetsSSBO.updateData(offsets.data(), offsets.size() * sizeof(int), slotIndex * sizeof(std::array<int,4>));

    const auto &vertices = chunk->getTransparentVertices();
    m_transparentSSBO.updateData(vertices.data(), vertices.size() * sizeof(BlockVertex), gpuSlot.vertexOffset * sizeof(BlockVertex));

    m_transparentHighestSlotUsed = std::max(m_transparentHighestSlotUsed, static_cast<size_t>(slotIndex) + 1);
}

void IndirectRenderer::addWaterChunk(const std::shared_ptr<Chunk> &chunk) {
    // Find a free GPU slot
    unsigned int slotIndex = UINT_MAX;
    for (unsigned int i = 0; i < m_waterGPUSlots.size(); i++) {
        if (!m_waterGPUSlots[i].isUsed) {
            slotIndex = i;
            break;
        }
    }

    if (slotIndex == UINT_MAX) {
        std::cerr << "No free GPU slot available!\n";
        return;
    }

    const unsigned int vertexCount = chunk->getWaterVertexCount();
    auto &gpuSlot = m_waterGPUSlots[slotIndex];
    gpuSlot.isUsed = true;
    gpuSlot.vertexCount = vertexCount;
    gpuSlot.vertexOffset = slotIndex * m_maxVerticesPerMesh;
    chunk->setGPUSlotWater(slotIndex);

    DrawArraysIndirectCommand cmd{};
    cmd.count = vertexCount;
    cmd.instanceCount = 1;
    cmd.first = 0;
    cmd.baseInstance = 0;

    m_waterIBO.updateData(&cmd, sizeof(cmd), slotIndex * sizeof(DrawArraysIndirectCommand));

    const std::array offsets = { // 4 integers for x, y, z, and a padding value
        chunk->getX(),
        chunk->getY(),
        chunk->getZ(),
        0
    };
    m_waterOffsetsSSBO.updateData(offsets.data(), offsets.size() * sizeof(int), slotIndex * sizeof(std::array<int,4>));

    const auto &vertices = chunk->getWaterVertices();
    m_waterSSBO.updateData(vertices.data(), vertices.size() * sizeof(BlockVertex), gpuSlot.vertexOffset * sizeof(BlockVertex));

    m_waterHighestSlotUsed = std::max(m_waterHighestSlotUsed, static_cast<size_t>(slotIndex) + 1);
}

void IndirectRenderer::removeOpaqueChunk(const std::shared_ptr<Chunk> &chunk) {
    const unsigned int slotIndex = chunk->getGPUSlotOpaque();
    auto &gpuSlot = m_opaqueGPUSlots[slotIndex];

    constexpr DrawArraysIndirectCommand emptyCmd{0, 0, 0, 0};
    m_opaqueIBO.updateData(&emptyCmd, sizeof(emptyCmd), slotIndex * sizeof(DrawArraysIndirectCommand));
    // No need to update SSBO or OffsetsSSBO, as they won't be used because draw command is zeroed

    gpuSlot.isUsed = false;
    gpuSlot.vertexOffset = 0;
    gpuSlot.vertexCount = 0;
    chunk->setGPUSlotOpaque(UINT_MAX);

    if (m_opaqueHighestSlotUsed > 0) {
        while (m_opaqueHighestSlotUsed > 0 && !m_opaqueGPUSlots[m_opaqueHighestSlotUsed - 1].isUsed) {
            --m_opaqueHighestSlotUsed;
        }
    }

}

void IndirectRenderer::removeTransparentChunk(const std::shared_ptr<Chunk> &chunk) {
    const unsigned int slotIndex = chunk->getGPUSlotTransparent();
    auto &gpuSlot = m_transparentGPUSlots[slotIndex];

    constexpr DrawArraysIndirectCommand emptyCmd{0, 0, 0, 0};
    m_transparentIBO.updateData(&emptyCmd, sizeof(emptyCmd), slotIndex * sizeof(DrawArraysIndirectCommand));
    // No need to update SSBO or OffsetsSSBO, as they won't be used because draw command is zeroed

    gpuSlot.isUsed = false;
    gpuSlot.vertexOffset = 0;
    gpuSlot.vertexCount = 0;
    chunk->setGPUSlotTransparent(UINT_MAX);

    if (m_transparentHighestSlotUsed > 0) {
        while (m_transparentHighestSlotUsed > 0 && !m_transparentGPUSlots[m_transparentHighestSlotUsed - 1].isUsed) {
            --m_transparentHighestSlotUsed;
        }
    }
}

void IndirectRenderer::removeWaterChunk(const std::shared_ptr<Chunk> &chunk) {
    const unsigned int slotIndex = chunk->getGPUSlotWater();
    auto &gpuSlot = m_waterGPUSlots[slotIndex];

    constexpr DrawArraysIndirectCommand emptyCmd{0, 0, 0, 0};
    m_waterIBO.updateData(&emptyCmd, sizeof(emptyCmd), slotIndex * sizeof(DrawArraysIndirectCommand));
    // No need to update SSBO or OffsetsSSBO, as they won't be used because draw command is zeroed

    gpuSlot.isUsed = false;
    gpuSlot.vertexOffset = 0;
    gpuSlot.vertexCount = 0;
    chunk->setGPUSlotWater(UINT_MAX);

    if (m_waterHighestSlotUsed > 0) {
        while (m_waterHighestSlotUsed > 0 && !m_waterGPUSlots[m_waterHighestSlotUsed - 1].isUsed) {
            --m_waterHighestSlotUsed;
        }
    }
}

void IndirectRenderer::updateOpaqueChunk(const std::shared_ptr<Chunk> &chunk) {
    const unsigned int slotIndex = chunk->getGPUSlotOpaque();
    auto &gpuSlot = m_opaqueGPUSlots[slotIndex];

    const unsigned int vertexCount = chunk->getOpaqueVertexCount();
    gpuSlot.vertexCount = vertexCount;

    DrawArraysIndirectCommand cmd{};
    cmd.count = vertexCount;
    cmd.instanceCount = 1;
    cmd.first = 0;
    cmd.baseInstance = 0;

    m_opaqueIBO.updateData(&cmd, sizeof(cmd), slotIndex * sizeof(DrawArraysIndirectCommand));

    const auto &vertices = chunk->getOpaqueVertices();
    m_opaqueSSBO.updateData(vertices.data(), vertices.size() * sizeof(BlockVertex), gpuSlot.vertexOffset * sizeof(BlockVertex));
    // Chunk doesn't change coordinates so no need to update offsets ssbo
}

void IndirectRenderer::updateTransparentChunk(const std::shared_ptr<Chunk> &chunk) {
    const unsigned int slotIndex = chunk->getGPUSlotTransparent();
    auto &gpuSlot = m_transparentGPUSlots[slotIndex];

    const unsigned int vertexCount = chunk->getTransparentVertexCount();
    gpuSlot.vertexCount = vertexCount;

    DrawArraysIndirectCommand cmd{};
    cmd.count = vertexCount;
    cmd.instanceCount = 1;
    cmd.first = 0;
    cmd.baseInstance = 0;

    m_transparentIBO.updateData(&cmd, sizeof(cmd), slotIndex * sizeof(DrawArraysIndirectCommand));

    const auto &vertices = chunk->getTransparentVertices();
    m_transparentSSBO.updateData(vertices.data(), vertices.size() * sizeof(BlockVertex), gpuSlot.vertexOffset * sizeof(BlockVertex));
    // Chunk doesn't change coordinates so no need to update offsets ssbo
}

void IndirectRenderer::updateWaterChunk(const std::shared_ptr<Chunk> &chunk) {
    const unsigned int slotIndex = chunk->getGPUSlotWater();
    auto &gpuSlot = m_waterGPUSlots[slotIndex];

    const unsigned int vertexCount = chunk->getWaterVertexCount();
    gpuSlot.vertexCount = vertexCount;

    DrawArraysIndirectCommand cmd{};
    cmd.count = vertexCount;
    cmd.instanceCount = 1;
    cmd.first = 0;
    cmd.baseInstance = 0;

    m_waterIBO.updateData(&cmd, sizeof(cmd), slotIndex * sizeof(DrawArraysIndirectCommand));

    const auto &vertices = chunk->getWaterVertices();
    m_waterSSBO.updateData(vertices.data(), vertices.size() * sizeof(BlockVertex), gpuSlot.vertexOffset * sizeof(BlockVertex));
    // Chunk doesn't change coordinates so no need to update offsets ssbo
}
