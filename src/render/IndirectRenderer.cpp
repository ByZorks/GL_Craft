#include "IndirectRenderer.h"

#include <iostream>

#include "Renderer.h"
#include "../world/Mesh.h"

IndirectRenderer::IndirectRenderer() {
    constexpr size_t totalCount = Chunk::SIZE * Chunk::SIZE * Chunk::SIZE * 0.5;
    m_IBO.init(nullptr, sizeof(DrawArraysIndirectCommand) * totalCount);
    m_SSBO.init(nullptr, totalCount * sizeof(BlockVertex), 1);
    m_offsetsSSBO.init(nullptr, totalCount * sizeof(std::array<int, 4>), 2);
}

void IndirectRenderer::createDrawCommands(const std::vector<std::shared_ptr<Chunk>> &opaqueMeshes, const std::vector<std::shared_ptr<Chunk>> &transparentMeshes, const std::vector<std::shared_ptr<Chunk>> &waterMeshes) {
    resetDrawCommands();

    m_opaqueCount = opaqueMeshes.size();
    m_transparentCount = transparentMeshes.size();
    m_waterCount = waterMeshes.size();
    const size_t totalCount = m_opaqueCount + m_transparentCount + m_waterCount;
    m_opaqueFirstCmd = 0;
    m_transparentFirstCmd = m_opaqueCount;
    m_waterFirstCmd = m_opaqueCount + m_transparentCount;
    size_t offset = 0;

    DrawArraysIndirectCommand cmds[totalCount];
    for (size_t localIndex = 0; localIndex < m_opaqueCount; ++localIndex) {
        cmds[localIndex].count = opaqueMeshes[localIndex]->getOpaqueVertexCount();
        cmds[localIndex].instanceCount = 1;
        cmds[localIndex].first = offset;
        cmds[localIndex].baseInstance = localIndex; // Used for vertex pulling
        offset += opaqueMeshes[localIndex]->getOpaqueVertexCount();
        const std::array<int, 4> offsets = { // 4 integers for x, y, z, and a padding value
            opaqueMeshes[localIndex]->getX(),
            opaqueMeshes[localIndex]->getY(),
            opaqueMeshes[localIndex]->getZ()
        };
        m_offsetsSSBO.updateData(offsets.data(), offsets.size() * sizeof(int), localIndex * sizeof(std::array<int,4>));
    }

    for (size_t localIndex = 0; localIndex < transparentMeshes.size(); ++localIndex) {
        const size_t globalIndex = m_opaqueCount + localIndex;
        cmds[globalIndex].count = transparentMeshes[localIndex]->getTransparentVertexCount();
        cmds[globalIndex].instanceCount = 1;
        cmds[globalIndex].first = offset;
        cmds[globalIndex].baseInstance = globalIndex;
        offset += transparentMeshes[localIndex]->getTransparentVertexCount();
        const std::array<int, 4> offsets = {
            transparentMeshes[localIndex]->getX(),
            transparentMeshes[localIndex]->getY(),
            transparentMeshes[localIndex]->getZ()
        };
        m_offsetsSSBO.updateData(offsets.data(), offsets.size() * sizeof(int), globalIndex * sizeof(std::array<int,4>));
    }

    for (size_t localIndex = 0; localIndex < waterMeshes.size(); ++localIndex) {
        const size_t globalIndex = m_opaqueCount + m_transparentCount + localIndex;
        cmds[globalIndex].count = waterMeshes[localIndex]->getWaterVertexCount();
        cmds[globalIndex].instanceCount = 1;
        cmds[globalIndex].first = offset;
        cmds[globalIndex].baseInstance = globalIndex;
        offset += waterMeshes[localIndex]->getWaterVertexCount();
        const std::array<int, 4> offsets = {
            waterMeshes[localIndex]->getX(),
            waterMeshes[localIndex]->getY(),
            waterMeshes[localIndex]->getZ()
        };
        m_offsetsSSBO.updateData(offsets.data(), offsets.size() * sizeof(int), globalIndex * sizeof(std::array<int,4>));
    }

    m_IBO.updateData(cmds, totalCount * sizeof(DrawArraysIndirectCommand), 0);

    std::vector<BlockVertex> allVertices;
    allVertices.reserve(totalCount);
    for (const auto& mesh : opaqueMeshes) {
        const auto& vertices = mesh->getOpaqueVertices();
        allVertices.insert(allVertices.end(), vertices.begin(), vertices.end());
    }
    for (const auto& mesh : transparentMeshes) {
        const auto& vertices = mesh->getTransparentVertices();
        allVertices.insert(allVertices.end(), vertices.begin(), vertices.end());
    }
    for (const auto& mesh : waterMeshes) {
        const auto& vertices = mesh->getWaterVertices();
        allVertices.insert(allVertices.end(), vertices.begin(), vertices.end());
    }

    m_SSBO.updateData(allVertices.data(), allVertices.size() * sizeof(BlockVertex));
}

void IndirectRenderer::resetDrawCommands() {
    m_opaqueCount = 0;
    m_transparentCount = 0;
    m_waterCount = 0;
    m_opaqueFirstCmd = 0;
    m_transparentFirstCmd = 0;
    m_waterFirstCmd = 0;
}

void IndirectRenderer::drawOpaque() const {
    if (!m_SSBO.isValid() || !m_offsetsSSBO.isValid()) return;
    Renderer::drawMultiWithVertexPulling(m_IBO, m_SSBO, m_offsetsSSBO, m_opaqueCount, reinterpret_cast<const void *>(m_opaqueFirstCmd * sizeof(DrawArraysIndirectCommand)));
}

void IndirectRenderer::drawTransparent() const {
    if (!m_SSBO.isValid() || !m_offsetsSSBO.isValid()) return;
    Renderer::drawMultiWithVertexPulling(m_IBO, m_SSBO, m_offsetsSSBO, m_transparentCount, reinterpret_cast<const void *>(m_transparentFirstCmd * sizeof(DrawArraysIndirectCommand)));
}

void IndirectRenderer::drawWater() const {
    if (!m_SSBO.isValid() || !m_offsetsSSBO.isValid()) return;
    Renderer::drawMultiWithVertexPulling(m_IBO, m_SSBO, m_offsetsSSBO, m_waterCount, reinterpret_cast<const void *>(m_waterFirstCmd * sizeof(DrawArraysIndirectCommand)));
}

