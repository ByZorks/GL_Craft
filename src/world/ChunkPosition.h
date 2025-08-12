#ifndef GL_CRAFT_CHUNKPOSITION_H
#define GL_CRAFT_CHUNKPOSITION_H
#include <tuple>

struct ChunkPosition {
    int x, y, z;

    friend bool operator==(const ChunkPosition &lhs, const ChunkPosition &rhs) {
        return std::tie(lhs.x, lhs.y, lhs.z) == std::tie(rhs.x, rhs.y, rhs.z);
    }

    friend bool operator!=(const ChunkPosition &lhs, const ChunkPosition &rhs) {
        return !(lhs == rhs);
    }
};

template <>
struct std::hash<ChunkPosition> {
    size_t operator()(const ChunkPosition& pos) const noexcept {
        const size_t h1 = hash<int>{}(pos.x);
        const size_t h2 = hash<int>{}(pos.y);
        const size_t h3 = hash<int>{}(pos.z);
        return h1 ^ h2 << 1 ^ h3;
    }
};

#endif //GL_CRAFT_CHUNKPOSITION_H