#ifndef GL_CRAFT_PLAYER_H
#define GL_CRAFT_PLAYER_H
#include "../world/Block.h"

class Player {
public:
    Player();

    [[nodiscard]] Block::BlockType getSelectedBlockType() const;
    void setBlockForCurrentlySelectedHotbarSlot(Block::BlockType blockType);
    void selectHotbarSlot(unsigned int hotbarIndex);

private:
    std::array<Block::BlockType, 9> m_blocks{};
    uint8_t m_selectedHotbarSlot = 0;
    Block::BlockType m_selectedBlockType = Block::BlockType::AIR;

};

#endif //GL_CRAFT_PLAYER_H