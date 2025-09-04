#include "Player.h"

Player::Player() = default;

Block::BlockType Player::getSelectedBlockType() const {
    return m_selectedBlockType;
}

void Player::setSelectedBlockType(const Block::BlockType blockType) {
    m_selectedBlockType = blockType;
}
