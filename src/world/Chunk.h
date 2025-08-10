#ifndef CHUNK_H
#define CHUNK_H
#include <unordered_set>

#include "Mesh.h"
#include "../utils/CustomHash.h"
#include "surfaceFeatures/SurfaceFeature.h"

class World;

class Chunk final : public Mesh {
private:
    std::unordered_set<SurfaceFeature> m_surfaceFeatures;

public:
    static constexpr unsigned int SIZE = 32;

    Chunk(int x, int y, int z);

    void generateVoxel(World &world);
    void generateMesh() override;

    [[nodiscard]] int index(int x, int y, int z) const override;
    [[nodiscard]] bool hasVisibleFaces() const;

    [[nodiscard]] const std::unordered_set<SurfaceFeature> & m_surface_features() const;

private:
    void addBlockFaces(int localX, int localY, int localZ, BlockType blockType);
    void addTree(int localX, int localY, int localZ);

    [[nodiscard]] BlockType getBlockType(int localX, int localY, int localZ) const override;
    [[nodiscard]] bool isBlockPresent(int localX, int localY, int localZ) const override;

};

#endif //CHUNK_H
