#ifndef CHUNK_H
#define CHUNK_H
#include <climits>
#include <list>
#include <mutex>
#include <random>
#include <unordered_map>
#include <unordered_set>

#include "ChunkLighting.h"
#include "ChunkPosition.h"
#include "Mesh.h"
#include "PendingBlock.h"
#include "PendingLight.h"
#include "../TerrainGenerator.h"
#include "../surfaceFeatures/SurfaceFeature.h"

class WorldManager;

class Chunk final : public Mesh {
public:
    static constexpr unsigned int SIZE = 32;

    Chunk(int x, int y, int z);

    void generateVoxel() override;
    void generatePendingBlocks(const std::list<PendingBlock> &blocks, MeshingResult &outResult);
    void generateMesh() override;
    void generateNewMesh(MeshingResult &outResult) const;
    void transferPendingBlocksToWorld(WorldManager &world);
    void transferPendingLightsToWorld(WorldManager &world);
    void deleteBlock(int localX, int localY, int localZ, Block::BlockType type, MeshingResult &outResult);
    void addBlock(int localX, int localY, int localZ, Block::BlockType type, MeshingResult &outResult);

    void propagateLight() const;
    void generatePendingLights(const std::list<PendingLight> &lights, MeshingResult &outResult) const;
    void emitBorderLights() const;

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
    template<typename NoiseFunction>
    void getDownsampledNoises(int factor, const std::span<float> &outNoises, NoiseFunction noiseFunction) const;
    void processColumn(int worldX, int worldZ, int localX, int localZ, const std::span<float> &tunnelCavesNoises,
                       const std::span<float> &largeCavesNoises);
    void generateSurfaceFeaturesPositions(const ChunkPosition &position, int worldX, int worldZ, int localX, int localZ,
                                          int columnHeight, Biome biome, const std::span<float> &tunnelCavesNoises,
                                          const std::span<float> &largeCavesNoises);
    void fillColumnBlocks(const ChunkPosition &position, int worldX, int worldZ, int localX, int localZ,
                          int columnHeight, Biome biome, const std::span<float> &tunnelCavesNoises,
                          const std::span<float> &largeCavesNoises);
    void addSurfaceFeatureBlocks(int worldX, int columnHeight, int worldZ, int localX, int localY, int localZ,
                                 Biome biome);

    void addBlockFaces(int localX, int localY, int localZ, Block::BlockType blockType);
    void addBlockFaces(int localX, int localY, int localZ, Block::BlockType blockType, MeshingResult &outResult) const;

    std::array<bool, 26> getAdjacentBlocksTransparency(int localX, int localY, int localZ, Block::BlockType blockType) const;

    [[nodiscard]] bool isBlockPresent(int localX, int localY, int localZ) const override;
    [[nodiscard]] bool hasVisibleFaces() const;

private:
    friend class ChunkLighting;
    ChunkLighting m_lighting;
    std::unordered_set<SurfaceFeature> m_surfaceFeatures;
    std::unordered_map<ChunkPosition, std::list<PendingBlock> > m_pendingBlocksForNeighbors;
    std::unordered_map<ChunkPosition, std::list<PendingLight> > m_pendingLightsForNeighbors;
    unsigned int m_drawIndexOpaque = UINT_MAX;
    unsigned int m_drawIndexWater = UINT_MAX;
    unsigned int m_indirectRendererSlotOpaque = UINT_MAX;
    unsigned int m_indirectRendererSlotWater = UINT_MAX;
    std::mt19937 m_rng;
};

#endif //CHUNK_H
