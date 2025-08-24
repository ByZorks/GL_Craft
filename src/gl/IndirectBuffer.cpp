#include "IndirectBuffer.h"

#include "OpenGLDebug.h"
#include "GL/glew.h"

IndirectBuffer::IndirectBuffer() = default;

IndirectBuffer::~IndirectBuffer() {
    deleteBuffer();
}

void IndirectBuffer::init(const void *data, const unsigned int size) {
    m_size = size;
    GLCall(glGenBuffers(1, &m_ID));
    bind();
    GLCall(glBufferData(GL_DRAW_INDIRECT_BUFFER, size, data, GL_STATIC_DRAW));
}

void IndirectBuffer::updateData(const void *data, const unsigned int size, const unsigned int offset) {
    if (m_size < size + offset) {
        deleteBuffer();
        init(data, size + offset);
    } else {
        bind();
        GLCall(glBufferSubData(GL_DRAW_INDIRECT_BUFFER, offset, size, data));
    }
}

void IndirectBuffer::deleteBuffer() {
    if (m_ID != 0) {
        GLCall(glDeleteBuffers(1, &m_ID));
        m_ID = 0;
        m_size = 0;
    }
}

void IndirectBuffer::bind() const {
    GLCall(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_ID));
}

void IndirectBuffer::unbind() {
    GLCall(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0));
}
