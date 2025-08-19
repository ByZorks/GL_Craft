#include "StorageBuffer.h"

#include "OpenGLDebug.h"
#include "GL/glew.h"

StorageBuffer::StorageBuffer() = default;

StorageBuffer::~StorageBuffer() {
    if (m_ID != 0) {
        GLCall(glDeleteBuffers(1, &m_ID));
    }
}

StorageBuffer::StorageBuffer(const StorageBuffer &other) = default;

StorageBuffer::StorageBuffer(StorageBuffer &&other) noexcept
        : m_ID(other.m_ID),
          m_bindingPoint(other.m_bindingPoint) {
    other.m_ID = 0;
    other.m_bindingPoint = 0;
}

StorageBuffer & StorageBuffer::operator=(const StorageBuffer &other) {
    if (this == &other)
        return *this;
    m_ID = other.m_ID;
    m_bindingPoint = other.m_bindingPoint;
    return *this;
}

StorageBuffer & StorageBuffer::operator=(StorageBuffer &&other) noexcept {
    if (this == &other)
        return *this;
    m_ID = other.m_ID;
    other.m_ID = 0;
    m_bindingPoint = other.m_bindingPoint;
    other.m_bindingPoint = 0;
    return *this;
}


void StorageBuffer::init(const void *data, const unsigned int size, const unsigned int bindingPoint) {
    m_bindingPoint = bindingPoint;
    GLCall(glGenBuffers(1, &m_ID));
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID));
    GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_STATIC_DRAW));
    GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_bindingPoint, m_ID));
}

void StorageBuffer::updateData(const void *data, const unsigned int size, const unsigned int offset) const {
    bind();
    GLCall(glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data));
    unbind();
}

void StorageBuffer::deleteBuffer() {
    if (m_ID != 0) {
        GLCall(glDeleteBuffers(1, &m_ID));
        m_ID = 0;
        m_bindingPoint = 999;
    }
}

void StorageBuffer::bind() const {
    GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_bindingPoint, m_ID));
}

void StorageBuffer::unbind() {
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
}
