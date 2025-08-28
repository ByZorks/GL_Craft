#ifndef GL_CRAFT_CROSSHAIR_H
#define GL_CRAFT_CROSSHAIR_H

#include <array>
#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"

#pragma pack(push, 1)
struct CrosshairVertex {
    float position[2]; // x, y
    uint8_t textureLayer; // Texture layer in the texture array
};
#pragma pack(pop)

class Crosshair {
private:
    constexpr static int m_layer = 23;
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    IndexBuffer m_IBO;
    constexpr static std::array<CrosshairVertex, 4> m_vertices = {
        // Top left, Top right, Bottom right, Bottom left
        CrosshairVertex{{-0.02f, 0.02f}, m_layer},
        CrosshairVertex{{0.02f, 0.02f}, m_layer},
        CrosshairVertex{{0.02f, -0.02f}, m_layer},
        CrosshairVertex{{-0.02f, -0.02f}, m_layer},
    };;
    constexpr static std::array<unsigned int, 6> m_indices = {
        0, 2, 1, 0, 3, 2
    };

public:
    Crosshair();

    void draw() const;

private:
    void createGLBuffers();
};

#endif //GL_CRAFT_CROSSHAIR_H