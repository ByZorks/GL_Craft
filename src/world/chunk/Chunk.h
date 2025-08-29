#ifndef CHUNK_H
#define CHUNK_H
#include <climits>
#include <unordered_map>
#include <unordered_set>

#include "ChunkPosition.h"
#include "Mesh.h"
#include "PendingBlock.h"
#include "../surfaceFeatures/SurfaceFeature.h"

class WorldManager;

class Chunk final : public Mesh {
private:
    std::unordered_set<SurfaceFeature> m_surfaceFeatures;
    std::unordered_map<ChunkPosition, std::vector<PendingBlock>> m_pendingBlocksForNeighbors;
    unsigned int m_opaqueDrawIndex = UINT_MAX;
    unsigned int m_waterDrawIndex = UINT_MAX;
    unsigned int m_gpuOpaqueSlot = UINT_MAX;
    unsigned int m_gpuWaterSlot = UINT_MAX;

public:
    static constexpr unsigned int SIZE = 32;

    Chunk(int x, int y, int z);

    void generateVoxel() override;
    void generatePendingBlocks(std::vector<PendingBlock> &blocks, MeshingResult &result);
    void generateMesh() override;
    void generateNewMesh(MeshingResult &result) const;
    void transferPendingBlocksToWorld(WorldManager &world);
    void deleteBlock(int localX, int localY, int localZ, BlockType type, MeshingResult &result);
    void addBlock(int localX, int localY, int localZ, BlockType type, MeshingResult &result);

    [[nodiscard]] int index(int x, int y, int z) const override;
    [[nodiscard]] BlockType getBlockType(int localX, int localY, int localZ) const override;
    [[nodiscard]] BlockType getBlockTypeOrSurfaceFeature(int localX, int localY, int localZ) const;
    [[nodiscard]] const std::unordered_set<SurfaceFeature> & getSurfaceFeatures() const;
    [[nodiscard]] unsigned int getOpaqueDrawIndex() const;
    void setOpaqueDrawIndex(unsigned int m_draw_index);
    [[nodiscard]] unsigned int getWaterDrawIndex() const;
    void setWaterDrawIndex(unsigned int m_water_draw_index);
    [[nodiscard]] unsigned int getGPUSlotOpaque() const;
    void setGPUSlotOpaque(unsigned int m_gpu_opaque_slot);
    [[nodiscard]] unsigned int getGPUSlotWater() const;
    void setGPUSlotWater(unsigned int m_gpu_water_slot);

private:
    void addBlockFaces(int localX, int localY, int localZ, BlockType blockType);
    void addBlockFaces(int localX, int localY, int localZ, BlockType blockType, MeshingResult &result) const;
    void addTree(int localX, int localY, int localZ);
    void addFeatureBlocks(int localX, int localY, int localZ, BlockType blockType);

    [[nodiscard]] bool isBlockPresent(int localX, int localY, int localZ) const override;
    [[nodiscard]] bool hasVisibleFaces() const;

};

#endif //CHUNK_H
