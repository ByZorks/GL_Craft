#ifndef GL_CRAFT_PENDINGBLOCK_H
#define GL_CRAFT_PENDINGBLOCK_H

enum class BlockType : unsigned char;

struct PendingBlock {
    int localX, localY, localZ;
    BlockType blockType;
};

#endif //GL_CRAFT_PENDINGBLOCK_H
