#ifndef WORLD_H
#define WORLD_H
#include "generation/TerrainGenerator.h"
#include "WorldManager.h"
#include "../render/WorldRenderer.h"

class World {
public:
    World();

    void update(const Camera &camera, const Frustum &frustum);
    void draw(const Shader &blockShader, const Shader &waterShader, const Shader &instancesShader,
              unsigned int &drawCmd);

    [[nodiscard]] const WorldManager &getWorldManagerConst() const;
    [[nodiscard]] const WorldRenderer &getWorldRendererConst() const;
    [[nodiscard]] const TerrainGenerator &getTerrainGeneratorConst() const;
    [[nodiscard]] WorldManager &getWorldManager();
    [[nodiscard]] WorldRenderer &getWorldRenderer();
    [[nodiscard]] TerrainGenerator &getTerrainGenerator();

private:
    WorldManager m_worldManager;
    WorldRenderer m_worldRenderer;
};

#endif //WORLD_H
