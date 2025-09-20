#include "SurfaceFeature.h"

#include "../chunk/Chunk.h"

static int index(const int x, const int y, const int z) {
    constexpr int stride = static_cast<int>(Chunk::SIZE) + 2;
    return x * stride * stride + z * stride + y;
}

SurfaceFeature::SurfaceFeature(const int x, const int y, const int z, const SurfaceFeatureType type) : m_x(x), m_y(y),
    m_z(z), m_type(type) {
}

SurfaceFeature::SurfaceFeature(const int x, const int y, const int z) : m_x(x), m_y(y), m_z(z),
                                                                        m_type(SurfaceFeatureType::NONE) {
}

std::size_t hash_value(const SurfaceFeature &obj) {
    std::size_t seed = 0x6F11962A;
    seed ^= (seed << 6) + (seed >> 2) + 0x5BF05181 + static_cast<std::size_t>(obj.m_x);
    seed ^= (seed << 6) + (seed >> 2) + 0x6F7D0896 + static_cast<std::size_t>(obj.m_y);
    seed ^= (seed << 6) + (seed >> 2) + 0x1E030344 + static_cast<std::size_t>(obj.m_z);
    return seed;
}


SurfaceFeature::SurfaceFeatureType SurfaceFeature::getSurfaceFeatureType(const float noiseValue, const Block::BlockType &blockType,
                                                         const Biome biome) {
    using enum SurfaceFeatureType;
    using enum Biome;

    if (biome == JUNGLE) {
        if (noiseValue >= 0.75f && noiseValue < 0.77f) return TREE;
        if (noiseValue >= 0.70f && noiseValue < 0.71f) return BUSH;
        if (noiseValue >= 0.69f) return SHORT_GRASS;
        return NONE;
    }

    if (biome == DESERT) {
        if (noiseValue >= 0.9999f) return POND;
    }

    if (biome == TAIGA || biome == SNOWY_TAIGA) {
        if (noiseValue >= 0.85f) return TREE;
    }

    if (biome == FOREST) {
        if (noiseValue >= 0.82f) return TREE;
    }

    if (biome == PLAINS) {
        if (noiseValue >= 0.93f) return TREE;
    }

    if (noiseValue >= 0.8099f && noiseValue < 0.81f) return POND;

    if (blockType == Block::BlockType::GRASS || blockType == Block::BlockType::SNOW_GRASS) {
        if (noiseValue >= 0.88f) return TREE;
        if (noiseValue >= 0.7111f && noiseValue < 0.7112f) return BUSH;
        if (noiseValue >= 0.70f) return SHORT_GRASS;
        if (noiseValue >= 0.696f) return POPPY;
        if (noiseValue >= 0.693f) return CORNFLOWER;
        if (noiseValue >= 0.690f) return ALLIUM;
    } else if (blockType == Block::BlockType::SAND) {
        if (noiseValue >= 0.88f) return TREE;
    }

    return NONE;
}

Block::BlockType SurfaceFeature::getBlockTypeOfSurfaceFeature(const SurfaceFeatureType type) {
    switch (type) {
        case SurfaceFeatureType::NONE:
            return Block::BlockType::AIR;
        case SurfaceFeatureType::TREE:
            return Block::BlockType::OAK_LOG; // Assuming trees are made of logs
        case SurfaceFeatureType::SHORT_GRASS:
            return Block::BlockType::SHORT_GRASS;
        case SurfaceFeatureType::POPPY:
            return Block::BlockType::FLOWER_POPPY;
        case SurfaceFeatureType::CORNFLOWER:
            return Block::BlockType::FLOWER_CORNFLOWER;
        case SurfaceFeatureType::ALLIUM:
            return Block::BlockType::FLOWER_ALLIUM;
        default:
            return Block::BlockType::AIR;
    }
}

SurfaceFeature::SurfaceFeatureType SurfaceFeature::getSurfaceFeatureTypeFromBlockType(const Block::BlockType &blockType) {
    switch (blockType) {
        case Block::BlockType::SHORT_GRASS:
            return SurfaceFeatureType::SHORT_GRASS;
        case Block::BlockType::FLOWER_POPPY:
            return SurfaceFeatureType::POPPY;
        case Block::BlockType::FLOWER_CORNFLOWER:
            return SurfaceFeatureType::CORNFLOWER;
        case Block::BlockType::FLOWER_ALLIUM:
            return SurfaceFeatureType::ALLIUM;
        default:
            return SurfaceFeatureType::NONE;
    }
}

void SurfaceFeature::addTree(std::mt19937 &rng, const ChunkPosition &position, const int localX, const int localY,
                             const int localZ, const Biome biome,
                             std::vector<Block::BlockType> &outBlocks,
                             std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings) {
    switch (biome) {
        using enum Biome;
        case DESERT:
            addCactus(rng, position, localX, localY, localZ, outBlocks, outPendings);
            break;
        case JUNGLE: {
            std::bernoulli_distribution distribution(0.40);
            distribution(rng)
                ? addMegaJungleTree(rng, position, localX, localY, localZ, outBlocks, outPendings)
                : addSmallTree(rng, position, localX, localY, localZ, biome, outBlocks, outPendings);
            break;
        }
        case TAIGA:
        case SNOWY_TAIGA:
            addSpruceTree(rng, position, localX, localY, localZ, outBlocks, outPendings);
            break;
        default:
            addSmallTree(rng, position, localX, localY, localZ, biome, outBlocks, outPendings);
            break;
    }
}

void SurfaceFeature::addBush(std::mt19937 &rng, const ChunkPosition &position, const int localX, const int localY,
                             const int localZ,
                             const Biome biome, std::vector<Block::BlockType> &outBlocks,
                             std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings) {
    Block::BlockType leavesType;
    if (TerrainGenerator::isSnowBiome(biome)) {
        leavesType = Block::BlockType::SNOW_OAK_LEAVES;
    } else if (biome == Biome::JUNGLE) {
        leavesType = Block::BlockType::JUNGLE_LEAVES;
    } else {
        leavesType = Block::BlockType::OAK_LEAVES;
    }

    for (int y = 0; y < 2; ++y) {
        for (int x = -1; x <= 1; ++x) {
            for (int z = -1; z <= 1; ++z) {
                if (rng() & 1) continue; // Skip some blocks to make it look more natural
                addFeatureBlocks(position, localX + x, localY + y, localZ + z, leavesType, outBlocks, outPendings);
            }
        }
    }
}

void SurfaceFeature::addPond(std::mt19937 &rng, const ChunkPosition &position, const int localX, const int localY,
                             const int localZ,
                             const Biome biome, std::vector<Block::BlockType> &outBlocks,
                             std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings) {
    const int radiusX = 2 + static_cast<int>(rng() % 3); // Pond radius between 2 and 4 blocks
    const int radiusZ = 2 + static_cast<int>(rng() % 3); // Pond radius between 2 and 4 blocks
    const int depth = 1 + static_cast<int>(rng() % 2); // Pond depth between 1 and 2 blocks
    const Block::BlockType bottomType = TerrainGenerator::getNearSurfaceBlockType(biome);

    for (int y = -depth; y < 0; ++y) {
        for (int x = -radiusX; x <= radiusX; ++x) {
            for (int z = -radiusZ; z <= radiusZ; ++z) {
                if (x * x + z * z <= radiusX * radiusZ) {
                    addFeatureBlocks(position, localX + x, localY + y, localZ + z,
                                     y == depth - 1 ? bottomType : Block::BlockType::WATER, outBlocks, outPendings);
                }
            }
        }
    }
}

void SurfaceFeature::addFeatureBlocks(const ChunkPosition &position, const int localX, const int localY,
                                      const int localZ,
                                      Block::BlockType blockType, std::vector<Block::BlockType> &outBlocks,
                                      std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings) {
    if (localX >= 1 && localX <= Chunk::SIZE &&
        localY >= 1 && localY <= Chunk::SIZE &&
        localZ >= 1 && localZ <= Chunk::SIZE) {
        outBlocks[index(localX, localY, localZ)] = blockType;
    } else {
        const int worldX = position.x + localX - 1;
        const int worldY = position.y + localY - 1;
        const int worldZ = position.z + localZ - 1;

        const int chunkX = static_cast<int>(std::floor(static_cast<float>(worldX) / Chunk::SIZE)) * static_cast<int>(
                               Chunk::SIZE);
        const int chunkY = static_cast<int>(std::floor(static_cast<float>(worldY) / Chunk::SIZE)) * static_cast<int>(
                               Chunk::SIZE);
        const int chunkZ = static_cast<int>(std::floor(static_cast<float>(worldZ) / Chunk::SIZE)) * static_cast<int>(
                               Chunk::SIZE);

        const int newLocalX = worldX - chunkX + 1;
        const int newLocalY = worldY - chunkY + 1;
        const int newLocalZ = worldZ - chunkZ + 1;

        outPendings[{chunkX, chunkY, chunkZ}].emplace_back(newLocalX, newLocalY, newLocalZ, blockType);
    }
}

void SurfaceFeature::addSmallTree(std::mt19937 &rng, const ChunkPosition &position, const int localX, const int localY,
                                  const int localZ, const Biome biome,
                                  std::vector<Block::BlockType> &outBlocks,
                                  std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings) {
    Block::BlockType leavesType;
    if (TerrainGenerator::isSnowBiome(biome)) {
        leavesType = Block::BlockType::SNOW_OAK_LEAVES;
    } else if (biome == Biome::JUNGLE) {
        leavesType = Block::BlockType::JUNGLE_LEAVES;
    } else {
        leavesType = Block::BlockType::OAK_LEAVES;
    }

    const Block::BlockType logType = biome == Biome::JUNGLE
                                         ? Block::BlockType::JUNGLE_LOG
                                         : Block::BlockType::OAK_LOG;

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

void SurfaceFeature::addCactus(std::mt19937 &rng, const ChunkPosition &position, const int localX, const int localY,
                               const int localZ,
                               std::vector<Block::BlockType> &outBlocks,
                               std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings) {
    const int height = 3 + static_cast<int>(rng() % 3); // Cactus height between 3 and 5 blocks

    for (int y = 0; y < height; ++y) {
        addFeatureBlocks(position, localX, localY + y, localZ, Block::BlockType::CACTUS, outBlocks, outPendings);
    }
}

void SurfaceFeature::addMegaJungleTree(std::mt19937 &rng, const ChunkPosition &position, const int localX,
                                       const int localY, const int localZ,
                                       std::vector<Block::BlockType> &outBlocks,
                                       std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings) {
    const int height = 15 + static_cast<int>(rng() % 11); // Height between 15 and 25 blocks
    // Trunk: 2xheightx2
    for (int y = 0; y < height; ++y) {
        for (int x = -1; x <= 0; ++x) {
            for (int z = -1; z <= 0; ++z) {
                addFeatureBlocks(position, localX + x, localY + y, localZ + z, Block::BlockType::JUNGLE_LOG, outBlocks,
                                 outPendings);
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
                if (x * x + z * z <= radius * radius) {
                    // Circle
                    if (layer == 0 && std::abs(x) == radius && std::abs(z) == radius) {
                        if (rng() & 1) continue; // Skip some corners on the bottom layer
                    }
                    addFeatureBlocks(position, localX + x, y, localZ + z, Block::BlockType::JUNGLE_LEAVES, outBlocks,
                                     outPendings);
                }
            }
        }
    }
}

void SurfaceFeature::addSpruceTree(std::mt19937 &rng, const ChunkPosition &position, const int localX, const int localY,
                                   const int localZ, std::vector<Block::BlockType> &outBlocks,
                                   std::unordered_map<ChunkPosition, std::list<PendingBlock> > &outPendings) {
    const int height = 7 + static_cast<int>(rng() % 4); // Height between 7 and 10 blocks

    // Trunk: 1xheightx1
    for (int y = 0; y < height; ++y) {
        addFeatureBlocks(position, localX, localY + y, localZ, Block::BlockType::SPRUCE_LOG, outBlocks, outPendings);
    }

    // Leaves: 7xheight/2x7
    std::bernoulli_distribution distrib(0.2);
    const int leavesStartY = localY + height / 2;
    for (int y = leavesStartY; y < localY + height; y++) {
        const int layer = y - leavesStartY;
        const int radius = 3 - layer / 2; // Decreasing radius
        const int outerLimit = radius * radius;
        const int innerLimit = radius > 1 ? (radius - 1) * (radius - 1) : 0;
        for (int x = -radius; x <= radius; x++) {
            for (int z = -radius; z <= radius; z++) {
                // Circle
                if (const int dist2 = x * x + z * z;
                    dist2 <= outerLimit) {
                    // Skip some blocks on the outer ring to make shape less perfect
                    if (dist2 > innerLimit && distrib(rng)) continue;
                    addFeatureBlocks(position, localX + x, y, localZ + z, Block::BlockType::SPRUCE_LEAVES, outBlocks,
                                     outPendings);
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

SurfaceFeature::SurfaceFeatureType SurfaceFeature::getType() const {
    return m_type;
}
