#ifndef CHUNK_H
#define CHUNK_H
#include <climits>
#include <random>
#include <unordered_map>
#include <unordered_set>

#include "ChunkPosition.h"
#include "Mesh.h"
#include "PendingBlock.h"
#include "../TerrainGenerator.h"
#include "../surfaceFeatures/SurfaceFeature.h"

class WorldManager;

class Chunk final : public Mesh {
public:
    static constexpr unsigned int SIZE = 32;

    Chunk(int x, int y, int z);

    void generateVoxel() override;
    void generatePendingBlocks(std::vector<PendingBlock> &blocks, MeshingResult &result);
    void propagateLight();
    void generateMesh() override;
    void generateNewMesh(MeshingResult &result) const;
    void transferPendingBlocksToWorld(WorldManager &world);
    void deleteBlock(int localX, int localY, int localZ, Block::BlockType type, MeshingResult &result);
    void addBlock(int localX, int localY, int localZ, Block::BlockType type, MeshingResult &result);

    [[nodiscard]] int index(int x, int y, int z) const override;
    [[nodiscard]] Block::BlockType getBlockType(int localX, int localY, int localZ) const override;
    [[nodiscard]] Block::BlockType getBlockTypeOrSurfaceFeature(int localX, int localY, int localZ) const;
    [[nodiscard]] const std::unordered_set<SurfaceFeature> &getSurfaceFeatures() const;
    [[nodiscard]] unsigned int getOpaqueDrawIndex() const;
    void setOpaqueDrawIndex(unsigned int m_draw_index);
    [[nodiscard]] unsigned int getWaterDrawIndex() const;
    void setWaterDrawIndex(unsigned int m_water_draw_index);
    [[nodiscard]] unsigned int getIndirectRendererSlotOpaque() const;
    void setIndirectRendererSlotOpaque(unsigned int m_gpu_opaque_slot);
    [[nodiscard]] unsigned int getIndirectRendererSlotWater() const;
    void setIndirectRendererSlotWater(unsigned int m_gpu_water_slot);

private:
    void addBlockFaces(int localX, int localY, int localZ, Block::BlockType blockType);
    void addBlockFaces(int localX, int localY, int localZ, Block::BlockType blockType, MeshingResult &result) const;

    uint8_t getLightLevelAt(int localX, int localY, int localZ) const;
    void setLightLevelAt(int localX, int localY, int localZ, uint8_t lightLevel);

    [[nodiscard]] bool isBlockPresent(int localX, int localY, int localZ) const override;
    [[nodiscard]] bool hasVisibleFaces() const;

private:
    std::unordered_set<SurfaceFeature> m_surfaceFeatures;
    std::unordered_map<ChunkPosition, std::vector<PendingBlock> > m_pendingBlocksForNeighbors;
    unsigned int m_drawIndexOpaque = UINT_MAX;
    unsigned int m_drawIndexWater = UINT_MAX;
    unsigned int m_indirectRendererSlotOpaque = UINT_MAX;
    unsigned int m_indirectRendererSlotWater = UINT_MAX;
    std::mt19937 m_rng;
};

#endif //CHUNK_H
