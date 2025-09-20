#ifndef GL_CRAFT_WORLDRENDERER_H
#define GL_CRAFT_WORLDRENDERER_H
#include "../gl/Shader.h"
#include "Camera.h"
#include "IndirectRenderer.h"
#include "InstanceRenderer.h"

class WorldRenderer {
public:
    WorldRenderer();

    void updateVisibleChunks(const Camera& camera, const Frustum& frustum, const WorldManager& manager);
    void draw(const Shader &blockShader, const Shader &waterShader, const Shader &instancesShader, unsigned int &drawCmd);

    [[nodiscard]] IndirectRenderer & getIndirectRenderer();
    [[nodiscard]] unsigned int getVisibleChunksCount() const;

private:
    void drawInstances(const Shader &instancesShader, unsigned int &drawCmd);
    void sortChunks(const Frustum &frustum, const Camera &camera,
                    const std::unordered_map<ChunkPosition, std::shared_ptr<Chunk> > &loadedChunks,
                    bool needInstanceUpdate);
    void processChunkVisibility(const std::shared_ptr<Chunk> &chunk, bool isInFrustum);
    void addVisibleChunkInstances(const std::shared_ptr<Chunk> &chunk, const Camera &camera);

private:
    IndirectRenderer m_indirectRenderer;
    std::unordered_map<SurfaceFeature::SurfaceFeatureType, InstanceRenderer> m_instanceRenderers;
    unsigned int m_visibleChunksCount = 0;
};

#endif //GL_CRAFT_WORLDRENDERER_H