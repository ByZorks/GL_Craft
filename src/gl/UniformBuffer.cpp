#include "UniformBuffer.h"

#include "OpenGLDebug.h"
#include "glad/gl.h"

UniformBuffer::UniformBuffer() = default;

UniformBuffer::~UniformBuffer() {
    if (m_ID != 0) {
        GLCall(glDeleteBuffers(1, &m_ID));
    }
}

void UniformBuffer::init(const void *data, const unsigned int size, const unsigned int bindingPoint) {
    m_bindingPoint = bindingPoint;
    GLCall(glCreateBuffers(1 , &m_ID));
    GLCall(glNamedBufferData(m_ID, size, data, GL_DYNAMIC_DRAW));
    GLCall(glBindBufferRange(GL_UNIFORM_BUFFER, m_bindingPoint, m_ID, 0, size));
}

void UniformBuffer::updateData(const void *data, const unsigned int size, const unsigned int offset) const {
    GLCall(glNamedBufferSubData(m_ID, offset, size, data));
}

void UniformBuffer::deleteBuffer() {
    if (m_ID != 0) {
        GLCall(glDeleteBuffers(1, &m_ID));
        m_ID = 0;
        m_bindingPoint = 0;
    }
}

void UniformBuffer::bind() const {
    GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_ID));
}
