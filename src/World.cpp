#include "World.h"

World::World() = default;

World::~World() {
    for (const auto &chunk : m_chunks) {
        delete chunk;
    }
    m_chunks.clear();
}

void World::generate() {
    const int chunkSize = static_cast<int>(Chunk::m_size1());

    for (int x = -m_renderDistance * chunkSize; x < m_renderDistance * chunkSize; x += chunkSize) {
        for (int y = 0; y < m_height * chunkSize; y += chunkSize) {
            for (int z = -m_renderDistance * chunkSize; z < m_renderDistance * chunkSize; z += chunkSize) {
                m_chunks.push_back(new Chunk(x, y, z));
                m_chunks.back()->generate();
                m_chunks.back()->setupBuffers();
            }
        }
    }
}

unsigned int World::m_size1() const {
    return m_renderDistance;
}

const std::vector<Chunk *>& World::m_chunks1() {
    return m_chunks;
}
