#ifndef GL_CRAFT_PLAYER_H
#define GL_CRAFT_PLAYER_H
#include "../world/Block.h"

class Player {
public:
    Player();

    [[nodiscard]] Block::BlockType getSelectedBlockType() const;
    void setSelectedBlockType(Block::BlockType blockType);

private:
    Block::BlockType m_selectedBlockType = Block::BlockType::PURPLE_LIGHT;

};

#endif //GL_CRAFT_PLAYER_H