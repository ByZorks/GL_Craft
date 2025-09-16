#include "Player.h"

Player::Player() = default;

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
    m_selectedHotbarSlot = hotbarIndex;
}
