#include "Mesh.h"

Mesh::Mesh(const int x, const int y, const int z, const unsigned int size) : m_size(size), m_x(x), m_y(y), m_z(z),
                                                                             m_box(AABB(static_cast<float>(x),
                                                                                 static_cast<float>(y),
                                                                                 static_cast<float>(z),
                                                                                 static_cast<float>(x + static_cast<int>(size)),
                                                                                 static_cast<float>(y + static_cast<int>(size)),
                                                                                 static_cast<float>(z + static_cast<int>(size))
                                                                             )) {
    m_blocks.resize(m_size * m_size * m_size, Block::BlockType::AIR);
}

void Mesh::generateVoxel() {}
void Mesh::generateMesh() {}

void Mesh::updateVertexCount() {
    m_opaqueData.verticesCount = static_cast<unsigned int>(m_opaqueData.vertices.size() * 6); // Only 1 vertex is stored
    m_waterData.verticesCount = static_cast<unsigned int>(m_waterData.vertices.size() * 6);
}

void Mesh::resetMesh() {
    m_opaqueData.deleteMesh();
    m_waterData.deleteMesh();
}

bool Mesh::isEmpty() const {
    return m_visibleBlocks == 0;
}

bool Mesh::hasOpaqueFaces() const {
    return m_opaqueData.hasFaces;
}

void Mesh::setHasOpaqueFaces(const bool hasFaces) {
    m_opaqueData.hasFaces = hasFaces;
}

bool Mesh::hasWaterFaces() const {
    return m_waterData.hasFaces;
}

void Mesh::setHasWaterFaces(const bool hasFaces) {
    m_waterData.hasFaces = hasFaces;
}

bool Mesh::shouldDrawFace(int x, int y, int z, const Block::BlockType currentBlockType, const Block::Face face) const {
    switch (face) {
        case Block::Face::TOP: y++;
            break;
        case Block::Face::BOTTOM: y--;
            break;
        case Block::Face::FRONT: z++;
            break;
        case Block::Face::BACK: z--;
            break;
        case Block::Face::RIGHT: x++;
            break;
        case Block::Face::LEFT: x--;
            break;
        default: ;
    }

    if (!isBlockPresent(x, y, z)) return true; // Air block

    const Block::BlockType neighborType = getBlockType(x, y, z);
    const bool neighborTransparent = Block::isTransparent(neighborType);

    if (currentBlockType == Block::BlockType::OAK_LEAVES ||
        currentBlockType == Block::BlockType::SNOW_OAK_LEAVES ||
        currentBlockType == Block::BlockType::JUNGLE_LEAVES ||
        currentBlockType == Block::BlockType::SPRUCE_LEAVES
        && neighborTransparent)
        return true; // Leaves block, always draw face
    // Always draw water top face if neighbor is not water
    if (currentBlockType == Block::BlockType::WATER && face == Block::Face::TOP && neighborType != Block::BlockType::WATER) return true;
    if (currentBlockType == neighborType) return false; // Same block type, no need to draw face

    const bool currentTransparent = Block::isTransparent(currentBlockType);
    // Current block is transparent, neighbor is not, do not draw face
    if (currentTransparent && !neighborTransparent) return false;

    return currentTransparent != neighborTransparent; // Different transparency state, draw face
}

bool Mesh::isBlockPresent(const int localX, const int localY, const int localZ) const {
    if (localX < 0 || localY < 0 || localZ < 0 || localX >= m_size || localY >= m_size || localZ >= m_size) {
        return false;
    }
    return m_blocks[index(localX, localY, localZ)] != Block::BlockType::AIR;
}

Block::BlockType Mesh::getBlockType(const int localX, const int localY, const int localZ) const {
    if (localX < 0 || localY < 0 || localZ < 0 || localX >= m_size || localY >= m_size || localZ >= m_size) {
        return Block::BlockType::AIR;
    }
    return m_blocks[index(localX, localY, localZ)];
}

int Mesh::index(const int x, const int y, const int z) const {
    const int stride = static_cast<int>(m_size);
    return x * stride * stride + y * stride + z;
}

int Mesh::getX() const {
    return m_x;
}

int Mesh::getY() const {
    return m_y;
}

int Mesh::getZ() const {
    return m_z;
}

Mesh::State Mesh::getState() const {
    return m_state;
}

void Mesh::setState(const State state) {
    m_state = state;
}

const AABB & Mesh::getBoundingBox() const {
    return m_box;
}

void Mesh::setWasInFrustum(const bool isInFrustum) {
    m_wasInFrustum = isInFrustum;
}

bool Mesh::wasInFrustum() const {
    return m_wasInFrustum;
}

std::vector<Block::BlockVertex> Mesh::getOpaqueVerticesCopy() const {
    return m_opaqueData.vertices; // Only used for initializing instance rendering, so a copy is fine
}

const std::vector<Block::BlockVertex> & Mesh::getOpaqueVertices() const {
    return m_opaqueData.vertices;
}

std::vector<Block::BlockVertex> & Mesh::getOpaqueVertices() {
    return m_opaqueData.vertices;
}

const std::vector<Block::BlockVertex> & Mesh::getWaterVertices() const {
    return m_waterData.vertices;
}

std::vector<Block::BlockVertex> & Mesh::getWaterVertices() {
    return m_waterData.vertices;
}

unsigned int Mesh::getOpaqueVertexCount() const {
    return m_opaqueData.verticesCount;
}

unsigned int Mesh::getWaterVertexCount() const {
    return m_waterData.verticesCount;
}
