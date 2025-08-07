#include "RenderBuffer.h"

#include "../core/OpenGLDebug.h"
#include "GL/glew.h"

RenderBuffer::RenderBuffer(const int width, const int height) : m_width(width), m_height(height) {
    GLCall(glGenRenderbuffers(1, &m_ID));
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_ID));
    GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height));
}

RenderBuffer::~RenderBuffer() {
    if (m_ID != 0) {
        GLCall(glDeleteRenderbuffers(1, &m_ID));
    }
}

void RenderBuffer::bind() const {
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_ID));
}

void RenderBuffer::unbind() {
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
}

unsigned int RenderBuffer::m_id() const {
    return m_ID;
}

void RenderBuffer::set_m_id(unsigned int m_id) {
    m_ID = m_id;
}
