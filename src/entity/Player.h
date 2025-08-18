#ifndef GL_CRAFT_PLAYER_H
#define GL_CRAFT_PLAYER_H
#include "../world/Block.h"

class Player {
private:
    BlockType m_selectedBlockType = BlockType::AIR;

public:
    Player();

    [[nodiscard]] BlockType getSelectedBlockType() const;
    void setSelectedBlockType(BlockType blockType);
};

#endif //GL_CRAFT_PLAYER_H