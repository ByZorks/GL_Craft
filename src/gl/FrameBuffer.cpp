#include "FrameBuffer.h"

#include <iostream>

#include "../core/OpenGLDebug.h"
#include "GL/glew.h"

FrameBuffer::FrameBuffer(const int width, const int height) : m_Width(width), m_Height(height),
                                                              m_colorTexture(width, height), m_depthTexture(width, height, true) {
    GLCall(glGenFramebuffers(1, &m_ID));
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_ID));

    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture.m_id(), 0));
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture.m_id(), 0));

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
      m_colorTexture(other.m_colorTexture), m_depthTexture(other.m_depthTexture) {
    other.m_ID = 0;
    other.m_colorTexture.set_m_id(0);
    other.m_depthTexture.set_m_id(0);
}

FrameBuffer &FrameBuffer::operator=(FrameBuffer &&other) noexcept {
    if (this != &other) {
        if (m_ID != 0) {
            GLCall(glDeleteFramebuffers(1, &m_ID));
        }

        m_ID = other.m_ID;
        m_colorTexture.set_m_id(other.m_colorTexture.m_id());
        m_depthTexture.set_m_id(other.m_depthTexture.m_id());
        m_Width = other.m_Width;
        m_Height = other.m_Height;

        other.m_ID = 0;
        other.m_colorTexture.set_m_id(0);
        other.m_depthTexture.set_m_id(0);
    }
    return *this;
}

void FrameBuffer::bind() const {
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_ID));
}

void FrameBuffer::unbind() {
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

const Texture & FrameBuffer::m_color_texture() const {
    return m_colorTexture;
}

const Texture & FrameBuffer::m_depth_texture() const {
    return m_depthTexture;
}
