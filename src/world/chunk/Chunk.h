#ifndef CHUNK_H
#define CHUNK_H
#include <climits>
#include <functional>
#include <list>
#include <mutex>
#include <random>
#include <unordered_map>
#include <unordered_set>

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
    void generatePendingBlocks(std::list<PendingBlock> &blocks, MeshingResult &result);
    void propagateLight();
    void generateMesh() override;
    void generateNewMesh(MeshingResult &result) const;
    void transferPendingBlocksToWorld(WorldManager &world);
    void transferPendingLightsToWorld(WorldManager &world);
    void generatePendingLights(std::list<PendingLight> &lights, MeshingResult &result);
    void deleteBlock(int localX, int localY, int localZ, Block::BlockType type, MeshingResult &result);
    void addBlock(int localX, int localY, int localZ, Block::BlockType type, MeshingResult &result);
    void emitBorderLights();

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
    void getDownsampledNoises(int factor, std::span<float> &outNoises, NoiseFunction noiseFunction) const;
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
    void addBlockFaces(int localX, int localY, int localZ, Block::BlockType blockType, MeshingResult &result) const;

    static uint32_t packLightPos(int x, int y, int z);
    static std::tuple<int, int, int> unpackLightPos(uint32_t v);

    uint8_t getLightLevelAt(int localX, int localY, int localZ) const;
    void setLightLevelAt(int localX, int localY, int localZ, uint8_t lightLevel);

    [[nodiscard]] bool isBlockPresent(int localX, int localY, int localZ) const override;
    [[nodiscard]] bool hasVisibleFaces() const;

private:
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
