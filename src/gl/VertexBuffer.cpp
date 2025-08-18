#include "VertexBuffer.h"

#include "OpenGLDebug.h"
#include "GL/glew.h"

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

void VertexBuffer::init(const void *data, const unsigned int size, BufferUsage usage) {
    GLCall(glGenBuffers(1, &m_ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_ID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, static_cast<GLenum>(usage)));
}

void VertexBuffer::updateData(const void *data, const unsigned int size, const unsigned int offset) const {
    bind();
    GLCall(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
}

void VertexBuffer::deleteBuffer() {
    if (m_ID != 0) {
        GLCall(glDeleteBuffers(1, &m_ID));
        m_ID = 0;
    }
}

void VertexBuffer::bind() const {
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_ID));
}

void VertexBuffer::unbind() {
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}
