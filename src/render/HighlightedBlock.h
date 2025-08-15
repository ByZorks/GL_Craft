#ifndef GL_CRAFT_HIGHLIGHTEDBLOCK_H
#define GL_CRAFT_HIGHLIGHTEDBLOCK_H
#include <array>

#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"
#include "../world/Block.h"
#include "../world/Mesh.h"

class HighlightedBlock {
private:
    const int m_x, m_y, m_z;
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    IndexBuffer m_IBO;
    constexpr static std::array<HighlightedVertex, 8> m_vertices = {
        HighlightedVertex{{0, 0, 0}, {0, 0, 0}},
        HighlightedVertex{{1, 0, 0}, {0, 0, 0}},
        HighlightedVertex{{1, 1, 0}, {0, 0, 0}},
        HighlightedVertex{{0, 1, 0}, {0, 0, 0}},
        HighlightedVertex{{0, 0, 1}, {0, 0, 0}},
        HighlightedVertex{{1, 0, 1}, {0, 0, 0}},
        HighlightedVertex{{1, 1, 1}, {0, 0, 0}},
        HighlightedVertex{{0, 1, 1}, {0, 0, 0}},
    };
    constexpr static std::array<unsigned int, 24> m_indices = {
        0, 1, 1, 2, 2, 3, 3, 0, // Face avant
        4, 5, 5, 6, 6, 7, 7, 4, // Face arrière
        0, 4, 1, 5, 2, 6, 3, 7  // Arêtes entre avant et arrière
    };
    State m_state = State::UNINITIALIZED;

public:
    HighlightedBlock(int x, int y, int z);

    void createGLBuffers();
    void draw() const;

    [[nodiscard]] int getX() const;
    [[nodiscard]] int getY() const;
    [[nodiscard]] int getZ() const;

    [[nodiscard]] State getState() const;
};

#endif //GL_CRAFT_HIGHLIGHTEDBLOCK_H