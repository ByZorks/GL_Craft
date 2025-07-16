#ifndef WORLD_H
#define WORLD_H

#include <unordered_map>

#include "Chunk.h"
#include <vector>

// Custom hash for std::tuple<int, int, int>
template<>
struct std::hash<std::tuple<int, int, int>> {
    size_t operator()(const std::tuple<int, int, int>& t) const noexcept {
        const auto hash1 = std::hash<int>{}(std::get<0>(t));
        const auto hash2 = std::hash<int>{}(std::get<1>(t));
        const auto hash3 = std::hash<int>{}(std::get<2>(t));

        // A simple way to combine hashes
        size_t seed = 0;
        seed ^= hash1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

class World {
private:
    const int m_halfWidth = 8; // Half the number of chunks in the x and z dimensions
    const unsigned int m_height = 2; // Number of chunk chunks in the y dimension
    std::unordered_map<std::tuple<int, int, int>, Chunk*> m_chunks;

public:
    World();
    ~World();

    Chunk* getChunk(int chunkBaseX, int chunkBaseY, int chunkBaseZ) const;
    void createChunks();

    [[nodiscard]] unsigned int m_size1() const;
    [[nodiscard]] std::unordered_map<std::tuple<int, int, int>, Chunk *> & m_chunks1();
};

#endif //WORLD_H
