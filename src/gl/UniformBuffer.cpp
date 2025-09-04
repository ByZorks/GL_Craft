#include "UniformBuffer.h"

#include "glad/gl.h"

UniformBuffer::UniformBuffer() = default;

UniformBuffer::~UniformBuffer() {
    if (m_ID != 0) {
        glDeleteBuffers(1, &m_ID);
    }
}

void UniformBuffer::init(const void *data, const unsigned int size, const unsigned int bindingPoint) {
    m_bindingPoint = bindingPoint;
    glCreateBuffers(1, &m_ID);
    glNamedBufferData(m_ID, size, data, GL_DYNAMIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, m_bindingPoint, m_ID, 0, size);
}

void UniformBuffer::updateData(const void *data, const unsigned int size, const unsigned int offset) const {
    glNamedBufferSubData(m_ID, offset, size, data);
}

void UniformBuffer::deleteBuffer() {
    if (m_ID != 0) {
        glDeleteBuffers(1, &m_ID);
        m_ID = 0;
        m_bindingPoint = 0;
    }
}
