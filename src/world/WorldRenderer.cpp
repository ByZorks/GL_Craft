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

void WorldRenderer::sortChunks(const Frustum &frustum, const Camera &camera,
                               const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk> > &loadedChunks,
                               const bool needInstanceUpdate) {
    if (needInstanceUpdate) {
        for (auto &renderer: m_instanceRenderers | std::views::values) {
            if (renderer.getInstancesCount() > 0) {
                renderer.resetInstances();
            }
        }
    }

    for (const auto &chunk: loadedChunks | std::views::values) {
        if (chunk->getState() < Mesh::State::READY_TO_DRAW) continue;

        const bool isInFrutum = frustum.isAABBInFrustum(chunk->getBoundingBox());

        // Update indirect renderer when chunk exit frustum
        if (!isInFrutum && chunk->wasInFrustum()) m_indirectRenderer.removeChunk(chunk);

        // Update indirect renderer when chunk enter frustum
        if (isInFrutum) {
            if (!chunk->wasInFrustum()) m_indirectRenderer.addChunk(chunk);
            m_visibleChunksCount++;

            // Surface features
            if (needInstanceUpdate && camera.distanceToCamera(*chunk) < 512.0f) {
                // They are no longer visible at this distance event if we draw them
                for (const auto &feature: chunk->getSurfaceFeatures()) {
                    switch (feature.getType()) {
                        using enum SurfaceFeature::SurfaceFeatureType;
                        case SHORT_GRASS:
                            m_instanceRenderers.at(SHORT_GRASS).addInstance({
                                feature.getX() - 1, feature.getY(), feature.getZ() - 1
                            });
                            break;
                        case POPPY:
                            m_instanceRenderers.at(POPPY).addInstance({
                                feature.getX() - 1, feature.getY(), feature.getZ() - 1
                            });
                            break;
                        case CORNFLOWER:
                            m_instanceRenderers.at(CORNFLOWER).addInstance({
                                feature.getX() - 1, feature.getY(), feature.getZ() - 1
                            });
                            break;
                        case ALLIUM:
                            m_instanceRenderers.at(ALLIUM).addInstance({
                                feature.getX() - 1, feature.getY(), feature.getZ() - 1
                            });
                            break;
                        default:
                            break;
                    }
                }
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
