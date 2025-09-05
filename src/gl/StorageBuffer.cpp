#include "StorageBuffer.h"

#include <iostream>

#include "glad/gl.h"

StorageBuffer::StorageBuffer() = default;

StorageBuffer::~StorageBuffer() {
    if (m_ID != 0) {
        glDeleteBuffers(1, &m_ID);
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

StorageBuffer &StorageBuffer::operator=(const StorageBuffer &other) {
    if (this == &other)
        return *this;
    m_ID = other.m_ID;
    m_bindingPoint = other.m_bindingPoint;
    m_size = other.m_size;
    return *this;
}

StorageBuffer &StorageBuffer::operator=(StorageBuffer &&other) noexcept {
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


void StorageBuffer::init(const void *data, const size_t size, const unsigned int bindingPoint, const bool isDynamic) {
    m_bindingPoint = bindingPoint;
    m_size = size;
    m_isDynamic = isDynamic;
    glCreateBuffers(1, &m_ID);
    if (m_isDynamic) {
        glNamedBufferStorage(m_ID, static_cast<GLsizeiptr>(size), data, GL_DYNAMIC_STORAGE_BIT);
    } else {
        glNamedBufferStorage(m_ID, static_cast<GLsizeiptr>(size), nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT  | GL_MAP_COHERENT_BIT);
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_bindingPoint, m_ID);
}

void StorageBuffer::unmapBuffer() const {
    glUnmapNamedBuffer(m_ID);
}

void * StorageBuffer::mapBuffer() const {
    if (m_isDynamic) throw std::runtime_error("StorageBuffer::mapBuffer() called on a dynamic buffer");
    return glMapNamedBufferRange(m_ID, 0, static_cast<GLsizeiptr>(m_size), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
}

size_t StorageBuffer::updateData(const void *data, const size_t size, const unsigned int offset) {
    if (m_size < size + offset) {
        resize(static_cast<size_t>(static_cast<double>(m_size) * 1.25));
        return m_size;
    }

    glNamedBufferSubData(m_ID, offset, static_cast<GLsizeiptr>(size), data);
    return 0;
}

void StorageBuffer::resize(const size_t newSize) {
    // std::cout << "[StorageBuffer " << m_ID << "] Resizing from " << m_size << " to " << newSize << " bytes.\n";
    const unsigned int oldID = m_ID;
    unsigned int newID = 0;

    // New buffer
    glCreateBuffers(1, &newID);
    if (m_isDynamic) {
        glNamedBufferStorage(newID, static_cast<GLsizeiptr>(newSize), nullptr, GL_DYNAMIC_STORAGE_BIT);
    } else {
        glNamedBufferStorage(newID, static_cast<GLsizeiptr>(newSize), nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT  | GL_MAP_COHERENT_BIT);
    }

    // Copy old data
    glCopyNamedBufferSubData(oldID, newID, 0, 0, static_cast<GLsizeiptr>(m_size));

    // Update members
    glDeleteBuffers(1, &oldID);
    m_ID = newID;
    m_size = newSize;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_bindingPoint, m_ID);
}

void StorageBuffer::deleteBuffer() {
    if (m_ID != 0) {
        glDeleteBuffers(1, &m_ID);
        m_ID = 0;
        m_bindingPoint = 999;
        m_size = 0;
    }
}

void StorageBuffer::bind() const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_bindingPoint, m_ID);
}

size_t StorageBuffer::getSize() const {
    return m_size;
}
