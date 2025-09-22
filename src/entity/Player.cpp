#include "Player.h"

Player::Player() {
    using enum Block::BlockType;
    m_blocks[0] = RED_LIGHT;
    m_blocks[1] = GREEN_LIGHT;
    m_blocks[2] = BLUE_LIGHT;
    m_blocks[3] = PURPLE_LIGHT;
    m_blocks[4] = PINK_LIGHT;
    m_blocks[5] = YELLOW_LIGHT;
}

Block::BlockType Player::getSelectedBlockType() const {
    return m_selectedBlockType;
}

void Player::setBlockForCurrentlySelectedHotbarSlot(const Block::BlockType blockType) {
    m_blocks[m_selectedHotbarSlot] = blockType;
    m_selectedBlockType = blockType;
}

void Player::selectHotbarSlot(const unsigned int hotbarIndex) {
    if (hotbarIndex >= m_blocks.size()) return;
    m_selectedBlockType = m_blocks[hotbarIndex];
    m_selectedHotbarSlot = static_cast<uint8_t>(hotbarIndex);
}
