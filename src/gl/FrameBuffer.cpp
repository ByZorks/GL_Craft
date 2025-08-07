#include "FrameBuffer.h"

#include <iostream>

#include "../core/OpenGLDebug.h"
#include "GL/glew.h"

FrameBuffer::FrameBuffer(const int width, const int height) : m_Width(width), m_Height(height),
                                                              m_texture(width, height), m_RBO(width, height) {
    GLCall(glGenFramebuffers(1, &m_ID));
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_ID));

    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture.m_renderer_id(), 0));

    GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RBO.m_id()));

    if (const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER); status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[OpengGL] Framebuffer error: " << status << std::endl;
    }
}

FrameBuffer::~FrameBuffer() {
    if (m_ID != 0) {
        GLCall(glDeleteFramebuffers(1, &m_ID));
    }
}

FrameBuffer::FrameBuffer(FrameBuffer &&other) noexcept
    : m_ID(other.m_ID), m_Width(other.m_Width), m_Height(other.m_Height),
      m_texture(other.m_texture), m_RBO(other.m_RBO) {
    other.m_ID = 0;
    other.m_texture.set_m_renderer_id(0);
    other.m_RBO.set_m_id(0);
}

FrameBuffer &FrameBuffer::operator=(FrameBuffer &&other) noexcept {
    if (this != &other) {
        if (m_ID != 0) {
            GLCall(glDeleteFramebuffers(1, &m_ID));
        }

        m_ID = other.m_ID;
        m_texture.set_m_renderer_id(other.m_texture.m_renderer_id());
        m_RBO.set_m_id(other.m_RBO.m_id());
        m_Width = other.m_Width;
        m_Height = other.m_Height;

        other.m_ID = 0;
        other.m_texture.set_m_renderer_id(0);
        other.m_RBO.set_m_id(0);
    }
    return *this;
}

void FrameBuffer::bind() const {
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_ID));
}

void FrameBuffer::unbind() {
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

const Texture &FrameBuffer::m_texture1() const {
    return m_texture;
}
