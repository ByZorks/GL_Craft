#include "IndirectBuffer.h"

#include "OpenGLDebug.h"
#include "glad/gl.h"

IndirectBuffer::IndirectBuffer() = default;

IndirectBuffer::~IndirectBuffer() {
    deleteBuffer();
}

void IndirectBuffer::init(const void *data, const unsigned int size) {
    m_size = size;
    GLCall(glGenBuffers(1, &m_ID));
    bind();
    GLCall(glBufferData(GL_DRAW_INDIRECT_BUFFER, size, data, GL_DYNAMIC_DRAW));
}

void IndirectBuffer::updateData(const void *data, const unsigned int size, const unsigned int offset) {
    if (m_size < size + offset) {
        resize(m_size * 2);
    } else {
        bind();
        GLCall(glBufferSubData(GL_DRAW_INDIRECT_BUFFER, offset, size, data));
    }
}

void IndirectBuffer::resize(const unsigned int newSize) {
    // std::cout << "[IndirectBuffer] Resizing from " << m_size / 1024 << " KiB to " << newSize / 1024 << " KiB\n";
    const unsigned int oldID = m_ID;
    unsigned int newID = 0;

    // New buffer
    GLCall(glGenBuffers(1, &newID));
    GLCall(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, newID));
    GLCall(glBufferData(GL_DRAW_INDIRECT_BUFFER, newSize, nullptr, GL_DYNAMIC_DRAW));

    // Copy old data
    GLCall(glBindBuffer(GL_COPY_READ_BUFFER, oldID));
    GLCall(glBindBuffer(GL_COPY_WRITE_BUFFER, newID));
    GLCall(glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, m_size));

    // Update members
    GLCall(glDeleteBuffers(1, &oldID));
    m_ID = newID;
    m_size = newSize;
    GLCall(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_ID));
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
