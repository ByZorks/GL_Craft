#ifndef GL_CRAFT_PENDINGBLOCK_H
#define GL_CRAFT_PENDINGBLOCK_H

class Block;
enum class BlockType : unsigned char;

struct PendingBlock {
    int localX, localY, localZ;
    Block::BlockType blockType;
};

#endif //GL_CRAFT_PENDINGBLOCK_H
