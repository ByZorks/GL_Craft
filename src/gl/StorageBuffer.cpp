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
          m_bindingPoint(other.m_bindingPoint), m_size(other.m_size) {
    other.m_ID = 0;
    other.m_bindingPoint = 999;
    other.m_size = 0;
}

StorageBuffer & StorageBuffer::operator=(const StorageBuffer &other) {
    if (this == &other)
        return *this;
    m_ID = other.m_ID;
    m_bindingPoint = other.m_bindingPoint;
    m_size = other.m_size;
    return *this;
}

StorageBuffer & StorageBuffer::operator=(StorageBuffer &&other) noexcept {
    if (this == &other)
        return *this;
    m_ID = other.m_ID;
    other.m_ID = 0;
    m_bindingPoint = other.m_bindingPoint;
    other.m_bindingPoint = 999;
    m_size = other.m_size;
    other.m_size = 0;
    return *this;
}


void StorageBuffer::init(const void *data, const unsigned int size, const unsigned int bindingPoint) {
    m_bindingPoint = bindingPoint;
    m_size = size;
    GLCall(glGenBuffers(1, &m_ID));
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID));
    GLCall(glBufferStorage(GL_SHADER_STORAGE_BUFFER, m_size, data, GL_DYNAMIC_STORAGE_BIT));
    GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_bindingPoint, m_ID));
}

void StorageBuffer::updateData(const void *data, const unsigned int size, const unsigned int offset) {
    if (m_size < size + offset) {
        const unsigned int bindingPoint = m_bindingPoint; // Save the binding point before deleting the buffer
        deleteBuffer();
        init(data, size, bindingPoint);
    } else {
        bind();
        GLCall(glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data));
    }
}

void StorageBuffer::deleteBuffer() {
    if (m_ID != 0) {
        GLCall(glDeleteBuffers(1, &m_ID));
        m_ID = 0;
        m_bindingPoint = 999;
        m_size = 0;
    }
}

void StorageBuffer::bind() const {
    GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_bindingPoint, m_ID));
}

void StorageBuffer::unbind() {
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
}
