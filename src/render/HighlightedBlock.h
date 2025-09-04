#ifndef GL_CRAFT_HIGHLIGHTEDBLOCK_H
#define GL_CRAFT_HIGHLIGHTEDBLOCK_H
#include <array>

#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"
#include "../world/Block.h"

class HighlightedBlock {
public:
    HighlightedBlock();

    void draw() const;

private:
    void createGLBuffers();

private:
    struct HighlightedVertex {
        std::array<uint8_t, 3> position; // Position
        std::array<uint8_t, 3> color; // Color
    };

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
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7
    };
};

#endif //GL_CRAFT_HIGHLIGHTEDBLOCK_H
