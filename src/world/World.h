#ifndef WORLD_H
#define WORLD_H
#include "TerrainGenerator.h"
#include "WorldManager.h"
#include "WorldRenderer.h"

class World {
private:
    WorldManager m_worldManager;
    WorldRenderer m_worldRenderer;
    TerrainGenerator m_terrainGenerator;

public:
    World();

    void update(const Camera &camera, const Frustum &frustum);
    void draw(const Shader &blockShader, const Shader &waterShader, const Shader &instancesShader, unsigned int &drawCmd);

    [[nodiscard]] const WorldManager & getWorldManagerConst() const;
    [[nodiscard]] const WorldRenderer & getWorldRendererConst() const;
    [[nodiscard]] const TerrainGenerator & getTerrainGeneratorConst() const;

    [[nodiscard]] WorldManager & getWorldManager();
    [[nodiscard]] WorldRenderer & getWorldRenderer();
    [[nodiscard]] TerrainGenerator & getTerrainGenerator();
};

#endif //WORLD_H
