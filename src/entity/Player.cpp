
#include "Player.h"

Player::Player() = default;

BlockType Player::getSelectedBlockType() const {
    return m_selectedBlockType;
}

void Player::setSelectedBlockType(const BlockType blockType) {
    m_selectedBlockType = blockType;
}
