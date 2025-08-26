#include "VertexBuffer.h"

#include "OpenGLDebug.h"
#include "glad/gl.h"

VertexBuffer::VertexBuffer() = default;

VertexBuffer::~VertexBuffer() {
    if (m_ID != 0) {
        GLCall(glDeleteBuffers(1, &m_ID));
    }
}

VertexBuffer::VertexBuffer(const VertexBuffer &other) = default;

VertexBuffer & VertexBuffer::operator=(const VertexBuffer &other) {
    if (this == &other)
        return *this;
    m_ID = other.m_ID;
    return *this;
}

VertexBuffer::VertexBuffer(VertexBuffer &&other) noexcept
        : m_ID(other.m_ID) {
    other.m_ID = 0;
}

VertexBuffer & VertexBuffer::operator=(VertexBuffer &&other) noexcept {
    if (this == &other)
        return *this;
    m_ID = other.m_ID;
    other.m_ID = 0;
    return *this;
}

void VertexBuffer::init(const void *data, const unsigned int size) {
    GLCall(glCreateBuffers(1, &m_ID));
    GLCall(glNamedBufferData(m_ID, size * sizeof(unsigned int), data, GL_STATIC_DRAW));
}

void VertexBuffer::updateData(const void *data, const unsigned int size, const unsigned int offset) const {
    GLCall(glNamedBufferSubData(m_ID, offset, size, data));
}

void VertexBuffer::deleteBuffer() {
    if (m_ID != 0) {
        GLCall(glDeleteBuffers(1, &m_ID));
        m_ID = 0;
    }
}

unsigned int VertexBuffer::getID() const {
    return m_ID;
}
