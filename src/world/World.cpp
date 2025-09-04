#include "World.h"

World::World() = default;

void World::update(const Camera &camera, const Frustum &frustum) {
    m_worldManager.updateChunks(camera, m_worldRenderer.getIndirectRenderer());
    m_worldRenderer.updateVisibleChunks(camera, frustum, m_worldManager);
}

void World::draw(const Shader &blockShader, const Shader &waterShader, const Shader &instancesShader,
                 unsigned int &drawCmd) {
    m_worldRenderer.draw(blockShader, waterShader, instancesShader, drawCmd);
}

const WorldManager &World::getWorldManagerConst() const {
    return m_worldManager;
}

const WorldRenderer &World::getWorldRendererConst() const {
    return m_worldRenderer;
}

const TerrainGenerator &World::getTerrainGeneratorConst() const {
    return m_terrainGenerator;
}

WorldManager &World::getWorldManager() {
    return m_worldManager;
}

WorldRenderer &World::getWorldRenderer() {
    return m_worldRenderer;
}

TerrainGenerator &World::getTerrainGenerator() {
    return m_terrainGenerator;
}
