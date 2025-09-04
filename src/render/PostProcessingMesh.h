#ifndef GL_CRAFT_POSTPROCESSINGMESH_H
#define GL_CRAFT_POSTPROCESSINGMESH_H
#include <array>

#include "../gl/FrameBuffer.h"
#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"

class PostProcessingMesh {
public:
    PostProcessingMesh(int width, int height);

    void resize(int width, int height);
    void draw() const;

    [[nodiscard]] const FrameBuffer & getFBO() const;

private:
    int m_width{}, m_height{};
    std::array<int8_t, 16> m_vertices = {
        // x, y, u, v
        -1, 1, 0, 1, // Top-left
        1, 1, 1, 1, // Top-right
        1, -1, 1, 0, // Bottom-right
        -1, -1, 0, 0, // Bottom-left
    };
    std::array<unsigned int, 6> m_indices = {
        0, 2, 1,
        0, 3, 2
    };
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    IndexBuffer m_IBO;
    FrameBuffer m_FBO;
};

#endif //GL_CRAFT_POSTPROCESSINGMESH_H