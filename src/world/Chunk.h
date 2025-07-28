#ifndef CHUNK_H
#define CHUNK_H
#include <vector>

#include "Block.h"
#include "FastNoiseLite.h"

#include "Mesh.h"

class World;

class Chunk final : public Mesh {
private:

public:
    static constexpr unsigned int SIZE = 16;

    Chunk(int x, int y, int z);
    ~Chunk() override;

    void generateVoxel(const FastNoiseLite& noiseGenerator, const FastNoiseLite& surfaceVegetationGenerator, const FastNoiseLite& caveGenerator);
    void generateMesh() override;
    void setupBuffers() override;

    static int index(int x, int y, int z);

    [[nodiscard]] bool hasBlocks();
    [[nodiscard]] bool hasVisibleFaces() const;

    [[nodiscard]] const VertexArray & m_vao() const;
    [[nodiscard]] const IndexBuffer & m_ibo() const;
    [[nodiscard]] static unsigned int m_size1() ;
    [[nodiscard]] const AABB & m_box1() const;
    [[nodiscard]] int m_x1() const;
    [[nodiscard]] int m_y1() const;
    [[nodiscard]] int m_z1() const;
    [[nodiscard]] Status m_status1() const;

private:
    void addBlockFaces(int localX, int localY, int localZ, BlockType blockType);
    [[nodiscard]] bool shouldDrawFace(int localX, int localY, int localZ, bool currentTransparent) const;
    [[nodiscard]] BlockType getBlockType(int localX, int localY, int localZ) const;
    [[nodiscard]] bool isBlockPresent(int localX, int localY, int localZ) const;
};

#endif //CHUNK_H
