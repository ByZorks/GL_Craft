#ifndef GL_CRAFT_SURFACEFEATURE_H
#define GL_CRAFT_SURFACEFEATURE_H

enum class SurfaceFeatureType : uint8_t {
    NONE,
    TREE,
    SHORT_GRASS,
    POPPY,
    CORNFLOWER,
    ALLIUM,
};

struct SurfaceFeature {
    int x, y, z; // Position in world coordinates
    SurfaceFeatureType type;

    SurfaceFeature(const int x, const int y, const int z, const SurfaceFeatureType type)
        : x(x), y(y), z(z), type(type) {}

    SurfaceFeature(const int x, const int y, const int z)
        : x(x), y(y), z(z), type(SurfaceFeatureType::NONE) {}

    friend bool operator==(const SurfaceFeature &lhs, const SurfaceFeature &rhs) {
        return std::tie(lhs.x, lhs.y, lhs.z) == std::tie(rhs.x, rhs.y, rhs.z);
    }

    friend bool operator!=(const SurfaceFeature &lhs, const SurfaceFeature &rhs) {
        return !(lhs == rhs);
    }
};

static SurfaceFeatureType getSurfaceFeatureType(const float noiseValue, const BlockType &blockType) {
    if (blockType == BlockType::GRASS || blockType == BlockType::SNOW_GRASS) {
        if (noiseValue >= 0.88f) return SurfaceFeatureType::TREE;
        if (noiseValue >= 0.70f) return SurfaceFeatureType::SHORT_GRASS;
        if (noiseValue >= 0.696f) return SurfaceFeatureType::POPPY;
        if (noiseValue >= 0.693f) return SurfaceFeatureType::CORNFLOWER;
        if (noiseValue >= 0.690f) return SurfaceFeatureType::ALLIUM;
    } else {
        // Nothing for now
    }

    return SurfaceFeatureType::NONE;
}

static BlockType getBlockTypeOfSurfaceFeature(const SurfaceFeatureType type) {
    switch (type) {
        case SurfaceFeatureType::NONE:
            return BlockType::AIR;
        case SurfaceFeatureType::TREE:
            return BlockType::LOG; // Assuming trees are made of logs
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

static SurfaceFeatureType getSurfaceFeatureTypeFromBlockType(const BlockType &blockType) {
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

template<>
struct std::hash<SurfaceFeature> {
    std::size_t operator()(const SurfaceFeature& obj) const noexcept {
        std::size_t seed = 0x73017B6B;
        seed ^= (seed << 6) + (seed >> 2) + 0x35E95AB9 + static_cast<std::size_t>(obj.x);
        seed ^= (seed << 6) + (seed >> 2) + 0x786F6256 + static_cast<std::size_t>(obj.y);
        seed ^= (seed << 6) + (seed >> 2) + 0x794EA429 + static_cast<std::size_t>(obj.z);
        return seed;
    }
};

#endif //GL_CRAFT_SURFACEFEATURE_H