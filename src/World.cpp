#include "World.h"

World::World() = default;

World::~World() {
    for (const auto &chunk : m_chunks) {
        delete chunk;
    }
    m_chunks.clear();
}

void World::generate() {
    for (int x = -m_renderDistance * 16; x < m_renderDistance * 16; x += 16) {
        for (int y = 0; y < m_height * 16; y += 16) {
            for (int z = -m_renderDistance * 16; z < m_renderDistance * 16; z += 16) {
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
