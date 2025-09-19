#include "Player.h"

Player::Player() {
    m_blocks[0] = Block::BlockType::RED_LIGHT;
    m_blocks[1] = Block::BlockType::GREEN_LIGHT;
    m_blocks[2] = Block::BlockType::BLUE_LIGHT;
    m_blocks[3] = Block::BlockType::PURPLE_LIGHT;
    m_blocks[4] = Block::BlockType::PINK_LIGHT;
    m_blocks[5] = Block::BlockType::YELLOW_LIGHT;
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
