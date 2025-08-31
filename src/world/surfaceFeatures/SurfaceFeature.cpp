#include "SurfaceFeature.h"

#include "../chunk/Chunk.h"

static int index(const int x, const int y, const int z) {
    constexpr int stride = static_cast<int>(Chunk::SIZE) + 2;
    return x * stride * stride + y * stride + z;
}

SurfaceFeature::SurfaceFeature(const int x, const int y, const int z, const SurfaceFeatureType type): m_x(x), m_y(y), m_z(z), m_type(type) {}

SurfaceFeature::SurfaceFeature(const int x, const int y, const int z): m_x(x), m_y(y), m_z(z), m_type(SurfaceFeatureType::NONE) {}

bool operator==(const SurfaceFeature &lhs, const SurfaceFeature &rhs) {
    return std::tie(lhs.m_x, lhs.m_y, lhs.m_z) == std::tie(rhs.m_x, rhs.m_y, rhs.m_z);
}

bool operator!=(const SurfaceFeature &lhs, const SurfaceFeature &rhs) {
    return !(lhs == rhs);
}

std::size_t hash_value(const SurfaceFeature &obj) {
    std::size_t seed = 0x6F11962A;
    seed ^= (seed << 6) + (seed >> 2) + 0x5BF05181 + static_cast<std::size_t>(obj.m_x);
    seed ^= (seed << 6) + (seed >> 2) + 0x6F7D0896 + static_cast<std::size_t>(obj.m_y);
    seed ^= (seed << 6) + (seed >> 2) + 0x1E030344 + static_cast<std::size_t>(obj.m_z);
    return seed;
}


SurfaceFeatureType SurfaceFeature::getSurfaceFeatureType(const float noiseValue, const BlockType &blockType, const Biome biome) {
    if (biome == Biome::JUNGLE) {
        if (noiseValue >= 0.75f && noiseValue < 0.77f) return SurfaceFeatureType::TREE;
        if (noiseValue >= 0.70f && noiseValue < 0.71f) return SurfaceFeatureType::BUSH;
        if (noiseValue >= 0.69f) return SurfaceFeatureType::SHORT_GRASS;
        return SurfaceFeatureType::NONE;
    }
    if (blockType == BlockType::GRASS || blockType == BlockType::SNOW_GRASS) {
        if (noiseValue >= 0.88f) return SurfaceFeatureType::TREE;
        if (noiseValue >= 0.7111f && noiseValue < 0.7112f) return SurfaceFeatureType::BUSH;
        if (noiseValue >= 0.70f) return SurfaceFeatureType::SHORT_GRASS;
        if (noiseValue >= 0.696f) return SurfaceFeatureType::POPPY;
        if (noiseValue >= 0.693f) return SurfaceFeatureType::CORNFLOWER;
        if (noiseValue >= 0.690f) return SurfaceFeatureType::ALLIUM;
    } else if (blockType == BlockType::SAND) {
        if (noiseValue >= 0.88f) return SurfaceFeatureType::TREE;
    }

    return SurfaceFeatureType::NONE;
}

BlockType SurfaceFeature::getBlockTypeOfSurfaceFeature(const SurfaceFeatureType type) {
    switch (type) {
        case SurfaceFeatureType::NONE:
            return BlockType::AIR;
        case SurfaceFeatureType::TREE:
            return BlockType::OAK_LOG; // Assuming trees are made of logs
        case SurfaceFeatureType::SHORT_GRASS:
            return BlockType::SHORT_GRASS;
        case SurfaceFeatureType::POPPY:
            return BlockType::FLOWER_POPPY;
        case SurfaceFeatureType::CORNFLOWER:
            return BlockType::FLOWER_CORNFLOWER;
        case SurfaceFeatureType::ALLIUM:
            return BlockType::FLOWER_ALLIUM;
        default:
            return BlockType::AIR;
    }
}

SurfaceFeatureType SurfaceFeature::getSurfaceFeatureTypeFromBlockType(const BlockType &blockType) {
    switch (blockType) {
        case BlockType::SHORT_GRASS:
            return SurfaceFeatureType::SHORT_GRASS;
        case BlockType::FLOWER_POPPY:
            return SurfaceFeatureType::POPPY;
        case BlockType::FLOWER_CORNFLOWER:
            return SurfaceFeatureType::CORNFLOWER;
        case BlockType::FLOWER_ALLIUM:
            return SurfaceFeatureType::ALLIUM;
        default:
            return SurfaceFeatureType::NONE;
    }
}

void SurfaceFeature::addTree(std::mt19937 &rng, const ChunkPosition &position, const int localX, const int localY, const int localZ, const Biome biome,
    std::vector<BlockType> &outBlocks, std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &outPendings) {
    switch (biome) {
        case Biome::DESERT:
            addCactus(rng, position, localX, localY, localZ, outBlocks, outPendings);
            break;
        case Biome::JUNGLE: {
            std::bernoulli_distribution distribution(0.40);
            distribution(rng)
                ? addMegaJungleTree(rng, position, localX, localY, localZ, outBlocks, outPendings)
                : addSmallTree(rng, position, localX, localY, localZ, biome, outBlocks, outPendings);
            break;
        }
        default:
            addSmallTree(rng, position, localX, localY, localZ, biome, outBlocks, outPendings);
            break;
    }
}

void SurfaceFeature::addBush(std::mt19937 &rng, const ChunkPosition &position, const int localX, const int localY, const int localZ,
    const Biome biome, std::vector<BlockType> &outBlocks,
    std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &outPendings) {
    const BlockType leavesType =
            TerrainGenerator::isSnowBiome(biome)
                ? BlockType::SNOW_OAK_LEAVES
                : biome == Biome::JUNGLE
                      ? BlockType::JUNGLE_LEAVES
                      : BlockType::OAK_LEAVES;

    for (int y = 0; y < 2; ++y) {
        for (int x = -1; x <= 1; ++x) {
            for (int z = -1; z <= 1; ++z) {
                if (rng() & 1) continue; // Skip some blocks to make it look more natural
                addFeatureBlocks(position, localX + x, localY + y, localZ + z, leavesType, outBlocks, outPendings);
            }
        }
    }
}

void SurfaceFeature::addFeatureBlocks(const ChunkPosition &position, const int localX, const int localY, const int localZ,
                                      BlockType blockType, std::vector<BlockType> &outBlocks,
                                      std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &outPendings) {
    if (localX >= 1 && localX <= Chunk::SIZE &&
        localY >= 1 && localY <= Chunk::SIZE &&
        localZ >= 1 && localZ <= Chunk::SIZE) {
        outBlocks[index(localX, localY, localZ)] = blockType;
    } else {
        const int worldX = position.x + localX - 1;
        const int worldY = position.y + localY - 1;
        const int worldZ = position.z + localZ - 1;

        const int chunkX = static_cast<int>(std::floor(static_cast<float>(worldX) / Chunk::SIZE)) * static_cast<int>(Chunk::SIZE);
        const int chunkY = static_cast<int>(std::floor(static_cast<float>(worldY) / Chunk::SIZE)) * static_cast<int>(Chunk::SIZE);
        const int chunkZ = static_cast<int>(std::floor(static_cast<float>(worldZ) / Chunk::SIZE)) * static_cast<int>(Chunk::SIZE);

        const int newLocalX = worldX - chunkX + 1;
        const int newLocalY = worldY - chunkY + 1;
        const int newLocalZ = worldZ - chunkZ + 1;

        outPendings[{chunkX, chunkY, chunkZ}].emplace_back(newLocalX, newLocalY, newLocalZ, blockType);
    }
}

void SurfaceFeature::addSmallTree(std::mt19937 &rng, const ChunkPosition &position, const int localX, const int localY, const int localZ, const Biome biome,
    std::vector<BlockType> &outBlocks, std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &outPendings) {
    const BlockType leavesType =
            TerrainGenerator::isSnowBiome(biome)
                ? BlockType::SNOW_OAK_LEAVES
                : biome == Biome::JUNGLE
                      ? BlockType::JUNGLE_LEAVES
                      : BlockType::OAK_LEAVES;

    const BlockType logType =
            biome == Biome::JUNGLE
                ? BlockType::JUNGLE_LOG
                : BlockType::OAK_LOG;

    // Trunk: 1x5x1 = 5 blocks (y=0 to y=4)
    for (int y = 0; y < 5; ++y) {
        addFeatureBlocks(position, localX, localY + y, localZ, logType, outBlocks, outPendings);
    }

    // Leaves: 5x2x5 = 50 blocks (y=3 to y=4)
    for (int y = 3; y < 5; y++) {
        for (int x = -2; x <= 2; x++) {
            for (int z = -2; z <= 2; z++) {
                if (x == 0 && z == 0) continue; // Skip the trunk position
                if (std::abs(x) == 2 && std::abs(z) == 2) {
                    if (rng() & 1) continue; // Skip some corners
                }
                addFeatureBlocks(position, localX + x, localY + y, localZ + z, leavesType, outBlocks, outPendings);
            }
        }
    }

    // Leaves: 3x2x3 = 18 blocks (y=5 to y=6)
    for (int y = 5; y < 7; y++) {
        for (int x = -1; x <= 1; x++) {
            for (int z = -1; z <= 1; z++) {
                if (y == 6 && std::abs(x) == 1 && std::abs(z) == 1) continue; // Skip high corners
                if (y == 5 && std::abs(x) == 1 && std::abs(z) == 1) {
                    if (rng() & 1) continue; // Skip some edges
                }
                addFeatureBlocks(position, localX + x, localY + y, localZ + z, leavesType, outBlocks, outPendings);
            }
        }
    }
}

void SurfaceFeature::addCactus(std::mt19937 &rng, const ChunkPosition &position, const int localX, const int localY, const int localZ,
    std::vector<BlockType> &outBlocks, std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &outPendings) {
    const int height = 3 + static_cast<int>(rng() % 3); // Cactus height between 3 and 5 blocks

    for (int y = 0; y < height; ++y) {
        addFeatureBlocks(position, localX, localY + y, localZ, BlockType::CACTUS, outBlocks, outPendings);
    }
}

void SurfaceFeature::addMegaJungleTree(std::mt19937 &rng, const ChunkPosition &position, const int localX, const int localY, const int localZ,
    std::vector<BlockType> &outBlocks, std::unordered_map<ChunkPosition, std::vector<PendingBlock>> &outPendings) {
    const int height = 15 + static_cast<int>(rng() % 11); // Height between 15 and 25 blocks
    // Trunk: 2xheightx2
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < 2; x++) {
            for (int z = 0; z < 2; z++) {
                addFeatureBlocks(position, localX + x, localY + y, localZ + z, BlockType::JUNGLE_LOG, outBlocks, outPendings);
            }
        }
    }

    // Leaves
    const int leavesStartY = localY + height - 4;
    for (int y = leavesStartY; y < leavesStartY + 4; y++) {
        const int layer = y - leavesStartY;
        const int radius = 5 - layer; // Decreasing radius
        for (int x = -radius; x <= radius + 1; x++) {
            for (int z = -radius; z <= radius + 1; z++) {
                if (x * x + z * z <= radius * radius) { // Circle
                    if (layer == 0 && std::abs(x) == radius && std::abs(z) == radius) {
                        if (rng() & 1) continue; // Skip some corners on the bottom layer
                    }
                    addFeatureBlocks(position, localX + x, y, localZ + z, BlockType::JUNGLE_LEAVES, outBlocks, outPendings);
                }
            }
        }
    }
}

bool SurfaceFeature::isMultiBlockFeature() const {
    return m_type == SurfaceFeatureType::TREE;
}

int SurfaceFeature::getX() const {
    return m_x;
}

int SurfaceFeature::getY() const {
    return m_y;
}

int SurfaceFeature::getZ() const {
    return m_z;
}

SurfaceFeatureType SurfaceFeature::getType() const {
    return m_type;
}
