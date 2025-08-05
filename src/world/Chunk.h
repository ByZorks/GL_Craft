#ifndef CHUNK_H
#define CHUNK_H
#include "Mesh.h"

class World;

class Chunk final : public Mesh {
public:
    static constexpr unsigned int SIZE = 16;

    Chunk(int x, int y, int z);

    void generateVoxel(World &world);
    void generateMesh() override;

    [[nodiscard]] int index(int x, int y, int z) const override;
    [[nodiscard]] bool hasBlocks();
    [[nodiscard]] bool hasVisibleFaces() const;

private:
    void addBlockFaces(int localX, int localY, int localZ, BlockType blockType);
    [[nodiscard]] BlockType getBlockType(int localX, int localY, int localZ) const override;
    [[nodiscard]] bool isBlockPresent(int localX, int localY, int localZ) const override;
};

#endif //CHUNK_H
