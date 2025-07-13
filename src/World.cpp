#include "World.h"

#include <algorithm>

World::World() = default;

World::~World() {
    for (const auto &chunk : m_chunks) {
        delete chunk;
    }
    m_chunks.clear();
}

void World::generate() {
    const int chunkSize = static_cast<int>(Chunk::m_size1());

    for (int x = -m_halfWidth * chunkSize; x < m_halfWidth * chunkSize; x += chunkSize) {
        for (int y = 0; y < m_height * chunkSize; y += chunkSize) {
            for (int z = -m_halfWidth * chunkSize; z < m_halfWidth * chunkSize; z += chunkSize) {
                m_chunks.push_back(new Chunk(x, y, z));
                m_chunks.back()->generate();
                m_chunks.back()->setupBuffers();
            }
        }
    }
}

void World::sortChunks(Camera &camera) {
    std::ranges::sort(m_chunks, [camera](const Chunk *a, const Chunk *b) {
        return camera.distanceToCamera(*a) < camera.distanceToCamera(*b);
    });
}

unsigned int World::m_size1() const {
    return m_halfWidth;
}

const std::vector<Chunk *>& World::m_chunks1() {
    return m_chunks;
}
