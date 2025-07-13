#ifndef WORLD_H
#define WORLD_H

#include "Chunk.h"
#include <vector>

#include "../render/Camera.h"

class World {
private:
    const int m_halfWidth = 8; // Half the number of chunks in the x and z dimensions
    const unsigned int m_height = 2; // Number of chunk chunks in the y dimension
    std::vector<Chunk*> m_chunks;

public:
    World();
    ~World();

    void generate();
    void sortChunks(Camera& camera);

    [[nodiscard]] unsigned int m_size1() const;

    [[nodiscard]] const std::vector<Chunk *>& m_chunks1();
};

#endif //WORLD_H
