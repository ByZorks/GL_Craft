#include "IndirectBuffer.h"

#include <iostream>

#include "glad/gl.h"

IndirectBuffer::IndirectBuffer() = default;

IndirectBuffer::~IndirectBuffer() {
    deleteBuffer();
}

void IndirectBuffer::init(const void *data, const unsigned int size) {
    m_size = size;
    glCreateBuffers(1, &m_ID);
    glNamedBufferData(m_ID, size, data, GL_DYNAMIC_DRAW);
}

void IndirectBuffer::updateData(const void *data, const unsigned int size, const unsigned int offset) {
    if (m_size < size + offset) {
        resize(m_size * 2);
    } else {
        glNamedBufferSubData(m_ID, offset, size, data);
    }
}

void IndirectBuffer::resize(const unsigned int newSize) {
    // std::cout << "[IndirectBuffer " << m_ID << "] Resizing from " << m_size / 1024 << " KiB to " << newSize / 1024 << " KiB\n";
    const unsigned int oldID = m_ID;
    unsigned int newID = 0;

    // New buffer
    glCreateBuffers(1, &newID);
    glNamedBufferData(newID, newSize, nullptr, GL_DYNAMIC_DRAW);

    // Copy old data
    glCopyNamedBufferSubData(oldID, newID, 0, 0, m_size);

    // Update members
    glDeleteBuffers(1, &oldID);
    m_ID = newID;
    m_size = newSize;
}

void IndirectBuffer::deleteBuffer() {
    if (m_ID != 0) {
        glDeleteBuffers(1, &m_ID);
        m_ID = 0;
        m_size = 0;
    }
}

void IndirectBuffer::bind() const {
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_ID);
}
