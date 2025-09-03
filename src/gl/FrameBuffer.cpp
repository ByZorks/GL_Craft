#include "FrameBuffer.h"

#include <iostream>

#include "glad/gl.h"

FrameBuffer::FrameBuffer(const int width, const int height) : m_Width(width), m_Height(height),
                                                              m_colorTexture(width, height), m_depthTexture(width, height, true) {
    glCreateFramebuffers(1, &m_ID);
    glNamedFramebufferTexture(m_ID, GL_COLOR_ATTACHMENT0, m_colorTexture.getID(), 0);
    glNamedFramebufferTexture(m_ID, GL_DEPTH_ATTACHMENT, m_depthTexture.getID(), 0);

    if (const auto status = glCheckNamedFramebufferStatus(m_ID, GL_FRAMEBUFFER); status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[OpengGL] Framebuffer error: " << status << std::endl;
    }
}

FrameBuffer::~FrameBuffer() {
    if (m_ID != 0) {
        glDeleteFramebuffers(1, &m_ID);
    }
}

FrameBuffer::FrameBuffer(FrameBuffer &&other) noexcept
    : m_ID(other.m_ID), m_Width(other.m_Width), m_Height(other.m_Height),
      m_colorTexture(other.m_colorTexture), m_depthTexture(other.m_depthTexture) {
    other.m_ID = 0;
    other.m_colorTexture.setID(0);
    other.m_depthTexture.setID(0);
}

FrameBuffer &FrameBuffer::operator=(FrameBuffer &&other) noexcept {
    if (this != &other) {
        if (m_ID != 0) {
            glDeleteFramebuffers(1, &m_ID);
        }

        m_ID = other.m_ID;
        m_colorTexture.setID(other.m_colorTexture.getID());
        m_depthTexture.setID(other.m_depthTexture.getID());
        m_Width = other.m_Width;
        m_Height = other.m_Height;

        other.m_ID = 0;
        other.m_colorTexture.setID(0);
        other.m_depthTexture.setID(0);
    }
    return *this;
}

void FrameBuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
}

void FrameBuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

const Texture & FrameBuffer::getColorTexture() const {
    return m_colorTexture;
}

const Texture & FrameBuffer::getDepthTexture() const {
    return m_depthTexture;
}
