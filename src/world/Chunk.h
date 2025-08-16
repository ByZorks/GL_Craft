#ifndef CHUNK_H
#define CHUNK_H
#include <unordered_map>
#include <unordered_set>

#include "ChunkPosition.h"
#include "Mesh.h"
#include "PendingBlock.h"
#include "surfaceFeatures/SurfaceFeature.h"

class World;

class Chunk final : public Mesh {
private:
    std::unordered_set<SurfaceFeature> m_surfaceFeatures;
    std::unordered_map<ChunkPosition, std::vector<PendingBlock>> m_pendingBlocksForNeighbors;

public:
    static constexpr unsigned int SIZE = 32;

    Chunk(int x, int y, int z);

    void generateVoxel() override;
    void generatePendingBlocks(std::vector<PendingBlock> &blocks);
    void generateMesh() override;
    void transferPendingBlocksToWorld(World &world);
    void deleteBlock(int localX, int localY, int localZ);

    [[nodiscard]] int index(int x, int y, int z) const override;
    [[nodiscard]] BlockType getBlockType(int localX, int localY, int localZ) const override;
    [[nodiscard]] bool hasVisibleFaces() const;

    [[nodiscard]] const std::unordered_set<SurfaceFeature> & getSurfaceFeatures() const;

private:
    void addBlockFaces(int localX, int localY, int localZ, BlockType blockType);
    void addTree(int localX, int localY, int localZ);
    void addFeatureBlocks(int localX, int localY, int localZ, BlockType blockType);

    [[nodiscard]] bool isBlockPresent(int localX, int localY, int localZ) const override;

};

#endif //CHUNK_H
