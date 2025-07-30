#ifndef CHUNK_H
#define CHUNK_H
#include <memory>
#include <vector>

#include "FastNoiseLite.h"

#include "Mesh.h"
#include "surface_vegetations/Vegetation.h"

class World;

class Chunk final : public Mesh {
private:
    std::vector<std::shared_ptr<Vegetation>> m_vegetations;

public:
    static constexpr unsigned int SIZE = 16;

    Chunk(int x, int y, int z);
    ~Chunk() override;

    void generateVoxel(World &world);
    void generateMesh() override;

    static int index(int x, int y, int z);

    [[nodiscard]] bool hasBlocks();
    [[nodiscard]] bool hasVisibleFaces() const;

    [[nodiscard]] const std::vector<std::shared_ptr<Vegetation>> & m_vegetations1() const;

private:
    void addBlockFaces(int localX, int localY, int localZ, BlockType blockType);
    [[nodiscard]] BlockType getBlockType(int localX, int localY, int localZ) const override;
    [[nodiscard]] bool isBlockPresent(int localX, int localY, int localZ) const override;
};

#endif //CHUNK_H
