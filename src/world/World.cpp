#include "World.h"

#include <ranges>

World::World() = default;

World::~World() {
    for (const auto &chunk: m_chunks | std::views::values) {
        delete chunk;
    }
    m_chunks.clear();
}

Chunk * World::getChunk(int chunkBaseX, int chunkBaseY, int chunkBaseZ) const {
    if (const auto key = std::make_tuple(chunkBaseX, chunkBaseY, chunkBaseZ); m_chunks.contains(key)) {
        return m_chunks.at(key);
    }
    return nullptr;
}

void World::createChunks() {
    const int chunkSize = static_cast<int>(Chunk::m_size1());

    // First pass: generate voxel data for each chunk
    for (int x = -m_halfWidth * chunkSize; x < m_halfWidth * chunkSize; x += chunkSize) {
        for (int y = 0; y < m_height * chunkSize; y += chunkSize) {
            for (int z = -m_halfWidth * chunkSize; z < m_halfWidth * chunkSize; z += chunkSize) {
                auto* chunk = new Chunk(x, y, z);
                chunk->generateVoxelData();
                m_chunks[std::make_tuple(x, y, z)] = chunk;
            }
        }
    }

    // Second pass: generate mesh data for each chunk and set up buffers
    for (const auto &chunk: m_chunks | std::views::values) {
        chunk->generateMeshData(this);
        chunk->setupBuffers();
    }
}

unsigned int World::m_size1() const {
    return m_halfWidth;
}

std::unordered_map<std::tuple<int, int, int>, Chunk *> & World::m_chunks1() {
    return m_chunks;
}
