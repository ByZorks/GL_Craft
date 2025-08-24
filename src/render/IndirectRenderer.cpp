#include "IndirectRenderer.h"

#include <cmath>
#include <iostream>

#include "Renderer.h"
#include "../world/Mesh.h"

IndirectRenderer::IndirectRenderer() {
    const auto renderDistanceInChunks = static_cast<size_t>(Renderer::s_renderDistance / Chunk::SIZE);
    const size_t chunksVisible = renderDistanceInChunks * renderDistanceInChunks * renderDistanceInChunks / 2; // Rough estimate
    constexpr size_t avgVerticesPerChunk = 5000; // Rough estimate
    const size_t slotsPerChunkEstimate = (avgVerticesPerChunk + m_vertexPerSlot - 1) / m_vertexPerSlot;
    const size_t nbSlotsMax = chunksVisible * slotsPerChunkEstimate * 2; // x2 for safety margin

    const size_t IBOSize = sizeof(DrawArraysIndirectCommand) * chunksVisible;
    const size_t SSBOSize = sizeof(BlockVertex) * m_vertexPerSlot * nbSlotsMax;
    const size_t offsetsSSBOSize = sizeof(std::array<int, 4>) * chunksVisible;

    // Opaque
    m_opaqueData.IBO.init(nullptr, IBOSize);
    m_opaqueData.verticesSSBO.init(nullptr, SSBOSize, 1);
    m_opaqueData.offsetsSSBO.init(nullptr, offsetsSSBOSize, 2);
    m_opaqueData.gpuSlots.resize(nbSlotsMax);

    // Transparent
    m_transparentData.IBO.init(nullptr, IBOSize / 5);
    m_transparentData.verticesSSBO.init(nullptr, SSBOSize / 5, 1);
    m_transparentData.offsetsSSBO.init(nullptr, offsetsSSBOSize / 5, 2);
    m_transparentData.gpuSlots.resize(nbSlotsMax / 5);

    // Water
    m_waterData.IBO.init(nullptr, IBOSize / 5);
    m_waterData.verticesSSBO.init(nullptr, SSBOSize / 5, 1);
    m_waterData.offsetsSSBO.init(nullptr, offsetsSSBOSize / 5, 2);
    m_waterData.gpuSlots.resize(nbSlotsMax / 5);
}

void IndirectRenderer::addChunk(const std::shared_ptr<Chunk> &chunk) {
    if (chunk->getGPUSlotOpaque() == UINT_MAX && chunk->hasOpaqueFaces()) add(m_opaqueData, MeshType::OPAQUE, chunk);
    if (chunk->getGPUSlotTransparent() == UINT_MAX && chunk->hasTransparentFaces()) add(m_transparentData, MeshType::TRANSPARENT, chunk);
    if (chunk->getGPUSlotWater() == UINT_MAX && chunk->hasWaterFaces()) add(m_waterData, MeshType::WATER, chunk);
}

void IndirectRenderer::removeChunk(const std::shared_ptr<Chunk> &chunk) {
    if (chunk->getGPUSlotOpaque() != UINT_MAX) remove(m_opaqueData, MeshType::OPAQUE, chunk);
    if (chunk->getGPUSlotTransparent() != UINT_MAX) remove(m_transparentData, MeshType::TRANSPARENT, chunk);
    if (chunk->getGPUSlotWater() != UINT_MAX) remove(m_waterData, MeshType::WATER, chunk);
}

void IndirectRenderer::updateChunk(const std::shared_ptr<Chunk> &chunk) {
    const bool hasOpaque = chunk->hasOpaqueFaces();
    const bool hasTransparent = chunk->hasTransparentFaces();
    const bool hasWater = chunk->hasWaterFaces();
    const unsigned int opaqueSlot = chunk->getGPUSlotOpaque();
    const unsigned int transparentSlot = chunk->getGPUSlotTransparent();
    const unsigned int waterSlot = chunk->getGPUSlotWater();

    // // Chunk can have new type of faces, so we may need to add it to a new slot
    if (opaqueSlot != UINT_MAX && hasOpaque) {
        update(m_opaqueData, MeshType::OPAQUE, chunk);
    } else if (opaqueSlot == UINT_MAX && hasOpaque) {
        add(m_opaqueData, MeshType::OPAQUE, chunk);
    }

    if (transparentSlot != UINT_MAX && hasTransparent) {
        update(m_transparentData, MeshType::TRANSPARENT, chunk);
    } else if (transparentSlot == UINT_MAX && hasTransparent) {
        add(m_transparentData, MeshType::TRANSPARENT, chunk);
    }

    if (waterSlot != UINT_MAX && hasWater) {
        update(m_waterData, MeshType::WATER, chunk);
    } else if (waterSlot == UINT_MAX && hasWater) {
        add(m_waterData, MeshType::WATER, chunk);
    }
}

void IndirectRenderer::drawOpaque() const {
    Renderer::drawMultiWithVertexPulling(m_opaqueData.IBO, m_opaqueData.verticesSSBO, m_opaqueData.offsetsSSBO, m_opaqueData.count, nullptr);
}

void IndirectRenderer::drawTransparent() const {
    Renderer::drawMultiWithVertexPulling(m_transparentData.IBO, m_transparentData.verticesSSBO, m_transparentData.offsetsSSBO, m_transparentData.count, nullptr);
}

void IndirectRenderer::drawWater() const {
    Renderer::drawMultiWithVertexPulling(m_waterData.IBO, m_waterData.verticesSSBO, m_waterData.offsetsSSBO, m_waterData.count, nullptr);
}

void IndirectRenderer::add(MeshData &meshData, const MeshType meshType, const std::shared_ptr<Chunk> &chunk) const {
    unsigned int vertexCount = 0;
    switch (meshType) {
        case MeshType::OPAQUE:
            vertexCount = chunk->getOpaqueVertexCount();
            break;
        case MeshType::TRANSPARENT:
            vertexCount = chunk->getTransparentVertexCount();
            break;
        case MeshType::WATER:
            vertexCount = chunk->getWaterVertexCount();
            break;
    }
    const unsigned int requiredSlots = (vertexCount + m_vertexPerSlot - 1) / m_vertexPerSlot;
    unsigned int foundSlots = 0;
    unsigned int startSlotIndex = UINT_MAX;

    // Find requireSlots consecutives free GPU slots
    for (unsigned int i = 0; i < meshData.gpuSlots.size(); i++) {
        if (!meshData.gpuSlots[i].isUsed) {
            if (foundSlots == 0) startSlotIndex = i; // Possible start
            foundSlots++;
            // If enough consecutive slots have been found, use them
            if (requiredSlots == foundSlots) break;
        } else {
            foundSlots = 0;
            startSlotIndex = UINT_MAX;
            if (const uint8_t &numberOfSlotsUsed = meshData.gpuSlots[i].numberOfSlotsUsed;
                numberOfSlotsUsed > 0) i += numberOfSlotsUsed - 1; // Skip used slots
        }
    }

    if (requiredSlots != foundSlots) {
        std::cerr << "No contiguous free GPU slots available!\n";
        return;
    }

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
            chunk->setGPUSlotOpaque(startSlotIndex);
            break;
        case MeshType::TRANSPARENT:
            chunk->setTransparentDrawIndex(drawIndex);
            chunk->setGPUSlotTransparent(startSlotIndex);
            break;
        case MeshType::WATER:
            chunk->setWaterDrawIndex(drawIndex);
            chunk->setGPUSlotWater(startSlotIndex);
            break;
    }
    chunk->setWasInFrustum(true);

    for (unsigned int i = 0; i < requiredSlots; ++i) {
        const unsigned int currentSlot = startSlotIndex + i;
        meshData.gpuSlots[currentSlot].isUsed = true;
        meshData.gpuSlots[currentSlot].numberOfSlotsUsed = i == 0 ? requiredSlots : 0;
    }

    DrawArraysIndirectCommand cmd{};
    cmd.count = vertexCount;
    cmd.instanceCount = 1;
    cmd.first = 0;
    cmd.baseInstance = startSlotIndex;

    meshData.IBO.updateData(&cmd, sizeof(cmd), drawIndex * sizeof(DrawArraysIndirectCommand));

    // 4 integers for x, y, z, and a padding value
    const std::array offsets = {chunk->getX(), chunk->getY(), chunk->getZ(), 0};
    meshData.offsetsSSBO.updateData(offsets.data(), offsets.size() * sizeof(int), drawIndex * sizeof(std::array<int,4>));

    const auto &vertices =
        meshType == MeshType::OPAQUE ? chunk->getOpaqueVertices() :
        meshType == MeshType::TRANSPARENT ? chunk->getTransparentVertices() :
        chunk->getWaterVertices();
    const size_t vertexBufferOffset = startSlotIndex * m_vertexPerSlot * sizeof(BlockVertex);
    if (const size_t newSize = meshData.verticesSSBO.updateData(vertices.data(), vertices.size() * sizeof(BlockVertex), vertexBufferOffset);
        newSize > 0) {
        const size_t newSlotCount = newSize / (m_vertexPerSlot * sizeof(BlockVertex));
        meshData.gpuSlots.resize(newSlotCount);
    }

    const size_t lastSlotUsed = startSlotIndex + requiredSlots - 1;
    meshData.highestSlotUsed = std::max(meshData.highestSlotUsed, lastSlotUsed);
}

void IndirectRenderer::remove(MeshData &meshData, const MeshType meshType, const std::shared_ptr<Chunk> &chunk) {
    unsigned int slotIndex = 0;
    unsigned int drawIndex = 0;
    switch (meshType) {
        case MeshType::OPAQUE:
            slotIndex = chunk->getGPUSlotOpaque();
            drawIndex = chunk->getOpaqueDrawIndex();
            break;
        case MeshType::TRANSPARENT:
            slotIndex = chunk->getGPUSlotTransparent();
            drawIndex = chunk->getTransparentDrawIndex();
            break;
        case MeshType::WATER:
            slotIndex = chunk->getGPUSlotWater();
            drawIndex = chunk->getWaterDrawIndex();
            break;

    }

    constexpr DrawArraysIndirectCommand emptyCmd{0, 0, 0, 0};
    meshData.IBO.updateData(&emptyCmd, sizeof(emptyCmd), drawIndex * sizeof(DrawArraysIndirectCommand));
    // No need to update SSBO or OffsetsSSBO, as they won't be used because draw command is zeroed

    auto &numberOfUsedSlots = meshData.gpuSlots[slotIndex].numberOfSlotsUsed;
    for (unsigned int i = 0; i < numberOfUsedSlots; ++i) {
        meshData.gpuSlots[slotIndex + i].isUsed = false;
    }
    numberOfUsedSlots = 0;

    meshData.freeDrawIndices.emplace(drawIndex);
    switch (meshType) {
        case MeshType::OPAQUE:
            chunk->setGPUSlotOpaque(UINT_MAX);
            chunk->setOpaqueDrawIndex(UINT_MAX);
            break;
        case MeshType::TRANSPARENT:
            chunk->setGPUSlotTransparent(UINT_MAX);
            chunk->setTransparentDrawIndex(UINT_MAX);
            break;
        case MeshType::WATER:
            chunk->setGPUSlotWater(UINT_MAX);
            chunk->setWaterDrawIndex(UINT_MAX);
            break;
    }
    chunk->setWasInFrustum(false);

    while (meshData.highestSlotUsed > 0 && !meshData.gpuSlots[meshData.highestSlotUsed - 1].isUsed) {
        --meshData.highestSlotUsed;
    }
}

void IndirectRenderer::update(MeshData &meshData, const MeshType meshType, const std::shared_ptr<Chunk> &chunk) const {
    unsigned int startSlotIndex = 0;
    unsigned int drawIndex = 0;
    unsigned int vertexCount = 0;
    switch (meshType) {
        case MeshType::OPAQUE:
            startSlotIndex = chunk->getGPUSlotOpaque();
            drawIndex = chunk->getOpaqueDrawIndex();
            vertexCount = chunk->getOpaqueVertexCount();
            break;
        case MeshType::TRANSPARENT:
            startSlotIndex = chunk->getGPUSlotTransparent();
            drawIndex = chunk->getTransparentDrawIndex();
            vertexCount = chunk->getTransparentVertexCount();
            break;
        case MeshType::WATER:
            startSlotIndex = chunk->getGPUSlotWater();
            drawIndex = chunk->getWaterDrawIndex();
            vertexCount = chunk->getWaterVertexCount();
            break;

    }

    const unsigned int newRequiredSlots = (vertexCount + m_vertexPerSlot - 1) / m_vertexPerSlot;
    const unsigned int oldRequiredSlots = meshData.gpuSlots[startSlotIndex].numberOfSlotsUsed;

    // Release unused slots
    if (newRequiredSlots < oldRequiredSlots) {
        for (unsigned int i = startSlotIndex + oldRequiredSlots - 1; i > startSlotIndex + newRequiredSlots - 1; --i) {
            meshData.gpuSlots[i].isUsed = false;
        }
        meshData.gpuSlots[startSlotIndex].numberOfSlotsUsed = newRequiredSlots;

    // Find new slots
    } else if (newRequiredSlots > oldRequiredSlots) {
        bool canExtend = true;
        for (unsigned int i = oldRequiredSlots; i < newRequiredSlots; ++i) {
            if (startSlotIndex + i >= meshData.gpuSlots.size() || meshData.gpuSlots[startSlotIndex + i].isUsed) {
                canExtend = false;
                break;
            }
        }

        if (canExtend) {
            for (unsigned int i = oldRequiredSlots; i < newRequiredSlots; ++i) {
                meshData.gpuSlots[startSlotIndex + i].isUsed = true;
            }
            meshData.gpuSlots[startSlotIndex].numberOfSlotsUsed = newRequiredSlots;
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
        meshType == MeshType::OPAQUE ? chunk->getOpaqueVertices() :
        meshType == MeshType::TRANSPARENT ? chunk->getTransparentVertices() :
        chunk->getWaterVertices();
    const size_t vertexBufferOffset = startSlotIndex * m_vertexPerSlot * sizeof(BlockVertex);
    if (const size_t newSize = meshData.verticesSSBO.updateData(vertices.data(), vertices.size() * sizeof(BlockVertex), vertexBufferOffset);
        newSize > 0) {
        const size_t newSlotCount = newSize / (m_vertexPerSlot * sizeof(BlockVertex));
        meshData.gpuSlots.resize(newSlotCount);
    }
    // Chunk doesn't change coordinates so no need to update offsets ssbo
}
