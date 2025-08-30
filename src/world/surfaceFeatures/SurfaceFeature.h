#ifndef GL_CRAFT_SURFACEFEATURE_H
#define GL_CRAFT_SURFACEFEATURE_H
#include <cstdint>
#include <random>
#include <unordered_map>

#include "../Block.h"
#include "../TerrainGenerator.h"
#include "../chunk/ChunkPosition.h"
#include "../chunk/PendingBlock.h"

enum class SurfaceFeatureType : uint8_t {
    NONE,
    TREE,
    SHORT_GRASS,
    POPPY,
    CORNFLOWER,
    ALLIUM,
};

class SurfaceFeature {
private:
    const int m_x, m_y, m_z; // Position in world coordinates
    const SurfaceFeatureType m_type;

public:
    SurfaceFeature(int x, int y, int z, SurfaceFeatureType type);
    SurfaceFeature(int x, int y, int z);

    friend bool operator==(const SurfaceFeature &lhs, const SurfaceFeature &rhs);
    friend bool operator!=(const SurfaceFeature &lhs, const SurfaceFeature &rhs);

    static SurfaceFeatureType getSurfaceFeatureType(float noiseValue, const BlockType &blockType);
    static BlockType getBlockTypeOfSurfaceFeature(SurfaceFeatureType type);
    static SurfaceFeatureType getSurfaceFeatureTypeFromBlockType(const BlockType &blockType);
    static void addTree(std::mt19937 &rng, const ChunkPosition &position, int localX, int localY, int localZ, Biome biome, std::vector<BlockType> &outBlocks, std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &outPendings);

    [[nodiscard]] bool isMultiBlockFeature() const;
    [[nodiscard]] int getX() const;
    [[nodiscard]] int getY() const;
    [[nodiscard]] int getZ() const;
    [[nodiscard]] SurfaceFeatureType getType() const;

private:
    friend std::size_t hash_value(const SurfaceFeature &obj);
    static void addFeatureBlocks(const ChunkPosition &position, int localX, int localY, int localZ, BlockType blockType, std::vector<BlockType> &outBlocks, std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &outPendings);

    static void addOakTree(std::mt19937 &rng, const ChunkPosition &position, int localX, int localY, int localZ, Biome biome, std::vector<BlockType> &outBlocks, std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &outPendings);
    static void addCactus(std::mt19937 &rng, const ChunkPosition &position, int localX, int localY, int localZ, std::vector<BlockType> &outBlocks, std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &outPendings);

};

template <>
struct std::hash<SurfaceFeature> {
    std::size_t operator()(const SurfaceFeature& obj) const noexcept {
        return hash_value(obj);
    }
};

#endif //GL_CRAFT_SURFACEFEATURE_H