#ifndef GL_CRAFT_FRAMEBUFFER_H
#define GL_CRAFT_FRAMEBUFFER_H
#include "RenderBuffer.h"
#include "Texture.h"

class FrameBuffer {
private:
    unsigned int m_ID = 0;
    int m_Width, m_Height;
    Texture m_texture;
    RenderBuffer m_RBO;

public:
    FrameBuffer(int width, int height);
    ~FrameBuffer();

    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;
    FrameBuffer(FrameBuffer&& other) noexcept;
    FrameBuffer& operator=(FrameBuffer&& other) noexcept;

    void bind() const;
    static void unbind();

    [[nodiscard]] const Texture & m_texture1() const;
};

#endif //GL_CRAFT_FRAMEBUFFER_H