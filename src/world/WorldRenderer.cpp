#include "WorldRenderer.h"

#include <ranges>

#include "WorldManager.h"
#include "../render/Renderer.h"
#include "surfaceFeatures/flowers/Allium.h"
#include "surfaceFeatures/flowers/Cornflower.h"
#include "surfaceFeatures/flowers/Poppy.h"
#include "surfaceFeatures/grass/ShortGrass.h"

WorldRenderer::WorldRenderer() {
    m_instanceRenderers.emplace(SurfaceFeature::SurfaceFeatureType::SHORT_GRASS, InstanceRenderer());
    m_instanceRenderers.emplace(SurfaceFeature::SurfaceFeatureType::POPPY, InstanceRenderer());
    m_instanceRenderers.emplace(SurfaceFeature::SurfaceFeatureType::CORNFLOWER, InstanceRenderer());
    m_instanceRenderers.emplace(SurfaceFeature::SurfaceFeatureType::ALLIUM, InstanceRenderer());

    m_instanceRenderers.at(SurfaceFeature::SurfaceFeatureType::SHORT_GRASS).init(std::move(ShortGrass(0, 0, 0)));
    m_instanceRenderers.at(SurfaceFeature::SurfaceFeatureType::POPPY).init(std::move(Poppy(0, 0, 0)));
    m_instanceRenderers.at(SurfaceFeature::SurfaceFeatureType::CORNFLOWER).init(std::move(Cornflower(0, 0, 0)));
    m_instanceRenderers.at(SurfaceFeature::SurfaceFeatureType::ALLIUM).init(std::move(Allium(0, 0, 0)));
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

    for (auto &renderer: m_instanceRenderers | std::views::values) {
        if (renderer.getInstancesCount() > 0) {
            anyVisible = true;
            break;
        }
    }

    if (!anyVisible) return;

    instancesShader.use();
    Renderer::disableBackFaceCulling();

    for (auto &renderer: m_instanceRenderers | std::views::values) {
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
                        case SurfaceFeature::SurfaceFeatureType::SHORT_GRASS:
                            m_instanceRenderers.at(SurfaceFeature::SurfaceFeatureType::SHORT_GRASS).addInstance({
                                feature.getX() - 1, feature.getY(), feature.getZ() - 1
                            });
                            break;
                        case SurfaceFeature::SurfaceFeatureType::POPPY:
                            m_instanceRenderers.at(SurfaceFeature::SurfaceFeatureType::POPPY).addInstance({
                                feature.getX() - 1, feature.getY(), feature.getZ() - 1
                            });
                            break;
                        case SurfaceFeature::SurfaceFeatureType::CORNFLOWER:
                            m_instanceRenderers.at(SurfaceFeature::SurfaceFeatureType::CORNFLOWER).addInstance({
                                feature.getX() - 1, feature.getY(), feature.getZ() - 1
                            });
                            break;
                        case SurfaceFeature::SurfaceFeatureType::ALLIUM:
                            m_instanceRenderers.at(SurfaceFeature::SurfaceFeatureType::ALLIUM).addInstance({
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
