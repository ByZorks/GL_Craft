#include "IndexBuffer.h"

#include <algorithm>

#include "glad/gl.h"

IndexBuffer::IndexBuffer() = default;

IndexBuffer::~IndexBuffer() {
    if (m_ID != 0) {
        glDeleteBuffers(1, &m_ID);
    }
}

IndexBuffer::IndexBuffer(const IndexBuffer &other) = default;

IndexBuffer & IndexBuffer::operator=(const IndexBuffer &other) {
    if (this == &other)
        return *this;
    m_ID = other.m_ID;
    m_Count = other.m_Count;
    return *this;
}

IndexBuffer::IndexBuffer(IndexBuffer &&other) noexcept
        : m_ID(other.m_ID),
          m_Count(other.m_Count) {
    other.m_ID = 0;
    other.m_Count = 0;
}

IndexBuffer & IndexBuffer::operator=(IndexBuffer &&other) noexcept {
    if (this == &other)
        return *this;
    m_ID = other.m_ID;
    other.m_ID = 0;
    m_Count = other.m_Count;
    other.m_Count = 0;
    return *this;
}

void IndexBuffer::init(const unsigned int *data, const unsigned int count) {
    m_Count = count;
    glCreateBuffers(1, &m_ID);
    glNamedBufferData(m_ID, static_cast<GLsizeiptr>(count * sizeof(unsigned int)), data, GL_STATIC_DRAW);
}

void IndexBuffer::updateData(const unsigned int *data) const {
    glNamedBufferSubData(m_ID, 0, static_cast<GLsizeiptr>(m_Count * sizeof(unsigned int)), data);
}

void IndexBuffer::deleteBuffer() {
    if (m_ID != 0) {
        glDeleteBuffers(1, &m_ID);
        m_ID = 0;
        m_Count = 0;
    }
}

unsigned int IndexBuffer::getCount() const {
    return m_Count;
}

unsigned int IndexBuffer::getID() const {
    return m_ID;
}
