#ifndef GL_CRAFT_CHUNKPOSITION_H
#define GL_CRAFT_CHUNKPOSITION_H

struct ChunkPosition {
    int x, y, z;

    friend bool operator==(const ChunkPosition &lhs, const ChunkPosition &rhs) = default;
};

template<>
struct std::hash<ChunkPosition> {
    size_t operator()(const ChunkPosition &pos) const noexcept {
        std::size_t seed = 0x32ECFE29;
        seed ^= (seed << 6) + (seed >> 2) + 0x6063D87C + static_cast<std::size_t>(pos.x);
        seed ^= (seed << 6) + (seed >> 2) + 0x72251FD8 + static_cast<std::size_t>(pos.y);
        seed ^= (seed << 6) + (seed >> 2) + 0x0D7EA89A + static_cast<std::size_t>(pos.z);
        return seed;
    }
};

#endif //GL_CRAFT_CHUNKPOSITION_H
