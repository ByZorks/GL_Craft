#include "WorldRenderer.h"

#include <ranges>

#include "WorldManager.h"
#include "../render/Renderer.h"
#include "surfaceFeatures/flowers/Allium.h"
#include "surfaceFeatures/flowers/Cornflower.h"
#include "surfaceFeatures/flowers/Poppy.h"
#include "surfaceFeatures/grass/ShortGrass.h"

WorldRenderer::WorldRenderer() {
    using enum SurfaceFeature::SurfaceFeatureType;
    m_instanceRenderers.try_emplace(SHORT_GRASS);
    m_instanceRenderers.try_emplace(POPPY);
    m_instanceRenderers.try_emplace(CORNFLOWER);
    m_instanceRenderers.try_emplace(ALLIUM);

    m_instanceRenderers.at(SHORT_GRASS).init(ShortGrass(0, 0, 0));
    m_instanceRenderers.at(POPPY).init(Poppy(0, 0, 0));
    m_instanceRenderers.at(CORNFLOWER).init(Cornflower(0, 0, 0));
    m_instanceRenderers.at(ALLIUM).init(Allium(0, 0, 0));
}

void WorldRenderer::updateVisibleChunks(const Camera &camera, const Frustum &frustum, const WorldManager &manager) {
    m_visibleChunksCount = 0;
    sortChunks(frustum, camera, manager.getLoadedChunks(), manager.getNeedInstanceUpdate());
}

void WorldRenderer::draw(const Shader &blockShader, const Shader &waterShader, const Shader &instancesShader,
                         unsigned int &drawCmd) {
    blockShader.use();
    m_indirectRenderer.drawOpaque();
    ++drawCmd;

    waterShader.use();
    Renderer::disableDepthMask();
    m_indirectRenderer.drawWater();
    ++drawCmd;
    Renderer::enableDepthMask();

    drawInstances(instancesShader, drawCmd);
}

IndirectRenderer &WorldRenderer::getIndirectRenderer() {
    return m_indirectRenderer;
}

unsigned int WorldRenderer::getVisibleChunksCount() const {
    return m_visibleChunksCount;
}

void WorldRenderer::drawInstances(const Shader &instancesShader, unsigned int &drawCmd) {
    bool anyVisible = false;

    for (const auto &renderer: m_instanceRenderers | std::views::values) {
        if (renderer.getInstancesCount() > 0) {
            anyVisible = true;
            break;
        }
    }

    if (!anyVisible) return;

    instancesShader.use();
    Renderer::disableBackFaceCulling();

    for (const auto &renderer: m_instanceRenderers | std::views::values) {
        if (renderer.getInstancesCount() > 0) {
            renderer.draw();
            ++drawCmd;
        }
    }

    Renderer::enableBackFaceCulling();
}

void WorldRenderer::sortChunks(const Frustum &frustum, const Camera &camera, const std::unordered_map<ChunkPosition,
    std::shared_ptr<Chunk>> &loadedChunks, const bool needInstanceUpdate) {
    if (needInstanceUpdate) {
        for (auto &renderer: m_instanceRenderers | std::views::values) {
            if (renderer.getInstancesCount() > 0) {
                renderer.resetInstances();
            }
        }
    }

    for (const auto &chunk: loadedChunks | std::views::values) {
        if (chunk->getState() < Mesh::State::READY_TO_DRAW) continue;

        const bool isInFrustum = frustum.isAABBInFrustum(chunk->getBoundingBox());
        processChunkVisibility(chunk, isInFrustum);

        if (isInFrustum) {
            m_visibleChunksCount++;
            if (needInstanceUpdate) {
                addVisibleChunkInstances(chunk, camera);
            }
        }
    }

    if (needInstanceUpdate) {
        for (auto &renderer: m_instanceRenderers | std::views::values) {
            if (renderer.getInstancesCount() > 0) {
                renderer.updateInstanceBuffer();
            }
        }
    }
}

void WorldRenderer::processChunkVisibility(const std::shared_ptr<Chunk> &chunk, const bool isInFrustum) {
    if (const bool wasInFrustum = chunk->wasInFrustum();
        isInFrustum && !wasInFrustum) {
        m_indirectRenderer.addChunk(chunk);
    } else if (!isInFrustum && wasInFrustum) {
        m_indirectRenderer.removeChunk(chunk);
    }
}

void WorldRenderer::addVisibleChunkInstances(const std::shared_ptr<Chunk> &chunk, const Camera &camera) {
    if (camera.distanceToCamera(*chunk) >= 512.0f) return;

    for (const auto &feature: chunk->getSurfaceFeatures()) {
        if (const auto type = feature.getType();
            m_instanceRenderers.contains(type)) {
            m_instanceRenderers.at(type).addInstance({
                feature.getX() - 1, feature.getY(), feature.getZ() - 1
            });
        }
    }
}
