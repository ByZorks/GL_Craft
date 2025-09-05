#include "IndirectRenderer.h"

#include <cmath>
#include <iostream>

#include "Renderer.h"
#include "../world/chunk/Mesh.h"

IndirectRenderer::IndirectRenderer() {
    const auto renderDistanceInChunks = static_cast<size_t>(Renderer::s_renderDistance / Chunk::SIZE);
    const size_t chunksVisible = renderDistanceInChunks * renderDistanceInChunks * renderDistanceInChunks / 2; // Rough estimate
    const size_t avgVerticesPerChunk = renderDistanceInChunks <= 16
                                                 ? 7000
                                                 : renderDistanceInChunks <= 32
                                                       ? 3000
                                                       : 1500; // Rough estimate
    const size_t slotsPerChunkEstimate = (avgVerticesPerChunk + m_vertexPerSlot - 1) / m_vertexPerSlot;
    const size_t nbSlotsMax = chunksVisible * slotsPerChunkEstimate * 2; // x2 for safety margin

    const size_t IBOSize = sizeof(DrawArraysIndirectCommand) * chunksVisible;
    const size_t SSBOSize = sizeof(Block::BlockVertex) * m_vertexPerSlot * nbSlotsMax;
    const size_t offsetsSSBOSize = sizeof(std::array<int, 3>) * chunksVisible;
    #if defined(DEBUG_BUILD) || defined(RELWITHDEBINFO_BUILD)
    std::cout << "[Indirect Renderer] IBOs total size: " << (IBOSize + IBOSize / 5) / 1024 << " KiB\n";
    std::cout << "[Indirect Renderer] Vertex SSBO size: " << SSBOSize / (1024 * 1024) << " MiB\n";
    std::cout << "[Indirect Renderer] Offsets SSBOs total size: " << (offsetsSSBOSize + offsetsSSBOSize / 5) / 1024 <<
            " KiB\n";
    #endif

    m_gpuSlots.resize(nbSlotsMax);

    // Use of mapping to avoid lots of big glBufferSubData calls that can result in ssbo being copied to ram
    m_verticesSSBO.init(nullptr, SSBOSize, 0, false);
    m_mappedVertices = static_cast<Block::BlockVertex *>(m_verticesSSBO.mapBuffer());
    if (!m_mappedVertices) throw std::runtime_error("IndirectRenderer: Failed to map vertex SSBO");

    // Opaque
    m_opaqueData.IBO.init(nullptr, IBOSize);
    m_opaqueData.offsetsSSBO.init(nullptr, offsetsSSBOSize, 1);

    // Water
    m_waterData.IBO.init(nullptr, IBOSize / 5);
    m_waterData.offsetsSSBO.init(nullptr, offsetsSSBOSize / 5, 1);
}

void IndirectRenderer::addChunk(const std::shared_ptr<Chunk> &chunk) {
    if (chunk->getIndirectRendererSlotOpaque() == UINT_MAX && chunk->hasOpaqueFaces()) add(m_opaqueData, MeshType::OPAQUE, chunk);
    if (chunk->getIndirectRendererSlotWater() == UINT_MAX && chunk->hasWaterFaces()) add(m_waterData, MeshType::WATER, chunk);
}

void IndirectRenderer::removeChunk(const std::shared_ptr<Chunk> &chunk) {
    if (chunk->getIndirectRendererSlotOpaque() != UINT_MAX) remove(m_opaqueData, MeshType::OPAQUE, chunk);
    if (chunk->getIndirectRendererSlotWater() != UINT_MAX) remove(m_waterData, MeshType::WATER, chunk);
}

void IndirectRenderer::updateChunk(const std::shared_ptr<Chunk> &chunk) {
    const bool hasOpaque = chunk->hasOpaqueFaces();
    const bool hasWater = chunk->hasWaterFaces();
    const unsigned int opaqueSlot = chunk->getIndirectRendererSlotOpaque();
    const unsigned int waterSlot = chunk->getIndirectRendererSlotWater();

    // Chunk can have new type of faces, so we may need to add it to a new slot
    if (opaqueSlot != UINT_MAX && hasOpaque) {
        update(m_opaqueData, MeshType::OPAQUE, chunk);
    } else if (opaqueSlot == UINT_MAX && hasOpaque) {
        add(m_opaqueData, MeshType::OPAQUE, chunk);
    }

    if (waterSlot != UINT_MAX && hasWater) {
        update(m_waterData, MeshType::WATER, chunk);
    } else if (waterSlot == UINT_MAX && hasWater) {
        add(m_waterData, MeshType::WATER, chunk);
    }
}

void IndirectRenderer::drawOpaque() const {
    if (m_opaqueData.count == 0) return;
    Renderer::drawMultiWithVertexPulling(m_opaqueData.IBO, m_verticesSSBO, m_opaqueData.offsetsSSBO, m_opaqueData.count,
                                         nullptr);
}

void IndirectRenderer::drawWater() const {
    if (m_waterData.count == 0) return;
    Renderer::drawMultiWithVertexPulling(m_waterData.IBO, m_verticesSSBO, m_waterData.offsetsSSBO, m_waterData.count,
                                         nullptr);
}

void IndirectRenderer::add(MeshData &meshData, const MeshType meshType, const std::shared_ptr<Chunk> &chunk) {
    unsigned int vertexCount = 0;
    switch (meshType) {
        case MeshType::OPAQUE:
            vertexCount = chunk->getOpaqueVertexCount();
            break;
        case MeshType::WATER:
            vertexCount = chunk->getWaterVertexCount();
            break;
    }
    const unsigned int requiredSlots = (vertexCount + m_vertexPerSlot - 1) / m_vertexPerSlot;
    unsigned int foundSlots = 0;
    unsigned int startSlotIndex = UINT_MAX;

    // Find "requireSlots" contiguous free GPU slots
    for (unsigned int i = 0; i < m_gpuSlots.size(); i++) {
        if (!m_gpuSlots[i].isUsed) {
            if (foundSlots == 0) startSlotIndex = i; // Possible start
            foundSlots++;
            // If enough consecutive slots have been found, use them
            if (requiredSlots == foundSlots) break;
        } else {
            foundSlots = 0;
            startSlotIndex = UINT_MAX;
            if (const uint8_t &numberOfSlotsUsed = m_gpuSlots[i].numberOfSlotsUsed;
                numberOfSlotsUsed > 0)
                i += numberOfSlotsUsed - 1; // Skip used slots
        }
    }

    if (requiredSlots != foundSlots) {
        const auto newSize = static_cast<size_t>(static_cast<double>(m_verticesSSBO.getSize()) * 1.25);
        const size_t oldSlotCount = m_gpuSlots.size();
        resizeVertexSSBOAndSlots(newSize);
        startSlotIndex = oldSlotCount;
    }

    // Get draw index
    unsigned int drawIndex;
    if (!meshData.freeDrawIndices.empty()) {
        drawIndex = meshData.freeDrawIndices.front();
        meshData.freeDrawIndices.pop();
    } else {
        drawIndex = meshData.count++;
    }

    switch (meshType) {
        case MeshType::OPAQUE:
            chunk->setOpaqueDrawIndex(drawIndex);
            chunk->setIndirectRendererSlotOpaque(startSlotIndex);
            break;
        case MeshType::WATER:
            chunk->setWaterDrawIndex(drawIndex);
            chunk->setIndirectRendererSlotWater(startSlotIndex);
            break;
    }
    chunk->setWasInFrustum(true);

    // Mark slots as used
    for (unsigned int i = 0; i < requiredSlots; ++i) {
        const unsigned int currentSlot = startSlotIndex + i;
        m_gpuSlots[currentSlot].isUsed = true;
        m_gpuSlots[currentSlot].numberOfSlotsUsed = i == 0 ? requiredSlots : 0;
    }

    DrawArraysIndirectCommand cmd{};
    cmd.count = vertexCount;
    cmd.instanceCount = 1;
    cmd.first = 0;
    cmd.baseInstance = startSlotIndex;

    meshData.IBO.updateData(&cmd, sizeof(cmd), drawIndex * sizeof(DrawArraysIndirectCommand));

    // 4 integers for x, y, z, and a padding value
    const std::array offsets = {chunk->getX(), chunk->getY(), chunk->getZ()};
    meshData.offsetsSSBO.updateData(offsets.data(), offsets.size() * sizeof(int),
                                    drawIndex * sizeof(std::array<int, 3>));

    const auto &vertices =
            meshType == MeshType::OPAQUE ? chunk->getOpaqueVertices() : chunk->getWaterVertices();
    const size_t vertexBufferOffset = startSlotIndex * m_vertexPerSlot;
    memcpy(m_mappedVertices + vertexBufferOffset, vertices.data(), vertices.size() * sizeof(Block::BlockVertex));

    const size_t lastSlotUsed = startSlotIndex + requiredSlots - 1;
    m_highestSlotUsed = std::max(m_highestSlotUsed, lastSlotUsed);
}

void IndirectRenderer::remove(MeshData &meshData, const MeshType meshType, const std::shared_ptr<Chunk> &chunk) {
    unsigned int slotIndex = 0;
    unsigned int drawIndex = 0;
    switch (meshType) {
        case MeshType::OPAQUE:
            slotIndex = chunk->getIndirectRendererSlotOpaque();
            drawIndex = chunk->getOpaqueDrawIndex();
            break;
        case MeshType::WATER:
            slotIndex = chunk->getIndirectRendererSlotWater();
            drawIndex = chunk->getWaterDrawIndex();
            break;
    }

    constexpr DrawArraysIndirectCommand emptyCmd{0, 0, 0, 0};
    meshData.IBO.updateData(&emptyCmd, sizeof(emptyCmd), drawIndex * sizeof(DrawArraysIndirectCommand));
    // No need to update vertices SSBO or offsets SSBO, as they won't be used because draw command is zeroed

    auto &numberOfUsedSlots = m_gpuSlots[slotIndex].numberOfSlotsUsed;
    for (unsigned int i = 0; i < numberOfUsedSlots; ++i) {
        m_gpuSlots[slotIndex + i].isUsed = false;
    }
    numberOfUsedSlots = 0;

    meshData.freeDrawIndices.emplace(drawIndex);
    switch (meshType) {
        case MeshType::OPAQUE:
            chunk->setIndirectRendererSlotOpaque(UINT_MAX);
            chunk->setOpaqueDrawIndex(UINT_MAX);
            break;
        case MeshType::WATER:
            chunk->setIndirectRendererSlotWater(UINT_MAX);
            chunk->setWaterDrawIndex(UINT_MAX);
            break;
    }
    chunk->setWasInFrustum(false);

    while (m_highestSlotUsed > 0 && !m_gpuSlots[m_highestSlotUsed - 1].isUsed) {
        --m_highestSlotUsed;
    }
}

void IndirectRenderer::update(MeshData &meshData, const MeshType meshType, const std::shared_ptr<Chunk> &chunk) {
    unsigned int startSlotIndex = 0;
    unsigned int drawIndex = 0;
    unsigned int vertexCount = 0;
    switch (meshType) {
        case MeshType::OPAQUE:
            startSlotIndex = chunk->getIndirectRendererSlotOpaque();
            drawIndex = chunk->getOpaqueDrawIndex();
            vertexCount = chunk->getOpaqueVertexCount();
            break;
        case MeshType::WATER:
            startSlotIndex = chunk->getIndirectRendererSlotWater();
            drawIndex = chunk->getWaterDrawIndex();
            vertexCount = chunk->getWaterVertexCount();
            break;
    }

    const unsigned int newRequiredSlots = (vertexCount + m_vertexPerSlot - 1) / m_vertexPerSlot;
    const unsigned int oldRequiredSlots = m_gpuSlots[startSlotIndex].numberOfSlotsUsed;

    // Release unused slots
    if (newRequiredSlots < oldRequiredSlots) {
        for (unsigned int i = startSlotIndex + oldRequiredSlots - 1; i > startSlotIndex + newRequiredSlots - 1; --i) {
            m_gpuSlots[i].isUsed = false;
        }
        m_gpuSlots[startSlotIndex].numberOfSlotsUsed = newRequiredSlots;

        // Find new slots
    } else if (newRequiredSlots > oldRequiredSlots) {
        bool canExtend = true;
        for (unsigned int i = oldRequiredSlots; i < newRequiredSlots; ++i) {
            if (startSlotIndex + i >= m_gpuSlots.size() || m_gpuSlots[startSlotIndex + i].isUsed) {
                canExtend = false;
                break;
            }
        }

        if (canExtend) {
            for (unsigned int i = oldRequiredSlots; i < newRequiredSlots; ++i) {
                m_gpuSlots[startSlotIndex + i].isUsed = true;
            }
            m_gpuSlots[startSlotIndex].numberOfSlotsUsed = newRequiredSlots;
        } else {
            remove(meshData, meshType, chunk);
            add(meshData, meshType, chunk);
            return;
        }
    }

    // Update if the number of slots didn't change or if we could extend
    DrawArraysIndirectCommand cmd{};
    cmd.count = vertexCount;
    cmd.instanceCount = 1;
    cmd.first = 0;
    cmd.baseInstance = startSlotIndex;

    meshData.IBO.updateData(&cmd, sizeof(cmd), drawIndex * sizeof(DrawArraysIndirectCommand));

    const auto &vertices =
            meshType == MeshType::OPAQUE ? chunk->getOpaqueVertices() : chunk->getWaterVertices();
    const size_t vertexBufferOffset = startSlotIndex * m_vertexPerSlot;
    memcpy(m_mappedVertices + vertexBufferOffset, vertices.data(), vertices.size() * sizeof(Block::BlockVertex));
    // Chunk doesn't change coordinates so no need to update offsets ssbo
}

void IndirectRenderer::resizeVertexSSBOAndSlots(const size_t ssboSize) {
    m_verticesSSBO.unmapBuffer();
    m_mappedVertices = nullptr;

    m_verticesSSBO.resize(ssboSize);
    #if defined(DEBUG_BUILD) || defined(RELWITHDEBINFO_BUILD)
    std::cout << "Resized vertex SSBO to " << ssboSize / (1024 * 1024) << " MiB\n";
    #endif

    m_mappedVertices = static_cast<Block::BlockVertex *>(m_verticesSSBO.mapBuffer());
    if (!m_mappedVertices) throw std::runtime_error("IndirectRenderer: Failed to map vertex SSBO");

    const size_t newSlotCount = ssboSize / (m_vertexPerSlot * sizeof(Block::BlockVertex));
    m_gpuSlots.resize(newSlotCount);
}
