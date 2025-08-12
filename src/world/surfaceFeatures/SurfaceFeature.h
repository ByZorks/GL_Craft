#ifndef GL_CRAFT_SURFACEFEATURE_H
#define GL_CRAFT_SURFACEFEATURE_H
#include <cstdint>

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

static SurfaceFeatureType getSurfaceFeatureType(const float noiseValue) {
    if (noiseValue >= 0.87f) return SurfaceFeatureType::TREE;
    if (noiseValue >= 0.70f) return SurfaceFeatureType::SHORT_GRASS;
    if (noiseValue >= 0.696f) return SurfaceFeatureType::POPPY;
    if (noiseValue >= 0.693f) return SurfaceFeatureType::CORNFLOWER;
    if (noiseValue >= 0.690f) return SurfaceFeatureType::ALLIUM;
    return SurfaceFeatureType::NONE;
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