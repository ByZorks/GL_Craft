#ifndef GL_CRAFT_SURFACEFEATURE_H
#define GL_CRAFT_SURFACEFEATURE_H
#include <cstdint>
#include <list>
#include <random>
#include <unordered_map>

#include "../Block.h"
#include "../TerrainGenerator.h"
#include "../chunk/ChunkPosition.h"
#include "../chunk/PendingBlock.h"

class SurfaceFeature {
public:
    enum class SurfaceFeatureType : uint8_t {
        NONE, TREE, SHORT_GRASS, POPPY, CORNFLOWER, ALLIUM, BUSH, POND
    };

public:
    SurfaceFeature(int x, int y, int z, SurfaceFeatureType type);
    SurfaceFeature(int x, int y, int z);

    friend bool operator==(const SurfaceFeature &lhs, const SurfaceFeature &rhs) noexcept {
        return lhs.m_x == rhs.m_x && lhs.m_y == rhs.m_y && lhs.m_z == rhs.m_z;
    }

    static SurfaceFeatureType getSurfaceFeatureType(float noiseValue, const Block::BlockType &blockType, Biome biome);
    static Block::BlockType getBlockTypeOfSurfaceFeature(SurfaceFeatureType type);
    static SurfaceFeatureType getSurfaceFeatureTypeFromBlockType(const Block::BlockType &blockType);
    static void addTree(std::mt19937 &rng, const ChunkPosition &position, int localX, int localY, int localZ,
                        Biome biome, std::vector<Block::BlockType> &outBlocks,
                        std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings);
    static void addBush(std::mt19937 &rng, const ChunkPosition &position, int localX, int localY, int localZ,
                        Biome biome, std::vector<Block::BlockType> &outBlocks,
                        std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings);
    static void addPond(std::mt19937 &rng, const ChunkPosition &position, int localX, int localY, int localZ,
                        Biome biome, std::vector<Block::BlockType> &outBlocks,
                        std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings);

    [[nodiscard]] bool isMultiBlockFeature() const;
    [[nodiscard]] int getX() const;
    [[nodiscard]] int getY() const;
    [[nodiscard]] int getZ() const;
    [[nodiscard]] SurfaceFeatureType getType() const;

private:
    friend std::size_t hash_value(const SurfaceFeature &obj);
    static void addFeatureBlocks(const ChunkPosition &position, int localX, int localY, int localZ, Block::BlockType blockType,
                                 std::vector<Block::BlockType> &outBlocks,
                                 std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings);
    static void addSmallTree(std::mt19937 &rng, const ChunkPosition &position, int localX, int localY, int localZ,
                             Biome biome, std::vector<Block::BlockType> &outBlocks,
                             std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings);
    static void addCactus(std::mt19937 &rng, const ChunkPosition &position, int localX, int localY, int localZ,
                          std::vector<Block::BlockType> &outBlocks,
                          std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings);
    static void addMegaJungleTree(std::mt19937 &rng, const ChunkPosition &position, int localX, int localY, int localZ,
                                  std::vector<Block::BlockType> &outBlocks,
                                  std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings);
    static void addSpruceTree(std::mt19937 &rng, const ChunkPosition &position, int localX, int localY, int localZ,
                              std::vector<Block::BlockType> &outBlocks,
                              std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings);

private:
    const int m_x, m_y, m_z; // Position in world coordinates
    const SurfaceFeatureType m_type;
};

template<>
struct std::hash<SurfaceFeature> {
    std::size_t operator()(const SurfaceFeature &obj) const noexcept {
        return hash_value(obj);
    }
};

#endif //GL_CRAFT_SURFACEFEATURE_H
