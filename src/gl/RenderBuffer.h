#ifndef GL_CRAFT_RENDERBUFFER_H
#define GL_CRAFT_RENDERBUFFER_H

class RenderBuffer {
private:
    unsigned int m_ID = 0;
    int m_width, m_height;

public:
    RenderBuffer(int width, int height);
    ~RenderBuffer();

    void bind() const;
    static void unbind();

    [[nodiscard]] unsigned int m_id() const;
    void set_m_id(unsigned int m_id);
};

#endif //GL_CRAFT_RENDERBUFFER_H