#ifndef GL_CRAFT_BLOCKSELECTOR_H
#define GL_CRAFT_BLOCKSELECTOR_H
#include <glm.hpp>
#include "HighlightedBlock.h"
#include "../gl/Shader.h"
#include "../math/Raycast.h"

class BlockSelector {
private:
    Shader* m_shader{};
    HighlightedBlock* m_mesh{};

public:
    BlockSelector();
    BlockSelector(Shader* shader, HighlightedBlock* mesh);

    void init(Shader* shader, HighlightedBlock* mesh);
    void render(const RaycastResult &hit, const glm::mat4 &mvp) const;
};

#endif //GL_CRAFT_BLOCKSELECTOR_H