#include "UniformBuffer.h"

#include "OpenGLDebug.h"
#include "GL/glew.h"

UniformBuffer::UniformBuffer() = default;

UniformBuffer::~UniformBuffer() {
    if (m_ID != 0) {
        GLCall(glDeleteBuffers(1, &m_ID));
    }
}

void UniformBuffer::init(const void *data, const unsigned int size, const unsigned int bindingPoint) {
    m_bindingPoint = bindingPoint;
    GLCall(glGenBuffers(1, &m_ID));
    GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_ID));
    GLCall(glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STATIC_DRAW));
    GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
    GLCall(glBindBufferRange(GL_UNIFORM_BUFFER, m_bindingPoint, m_ID, 0, size));
}

void UniformBuffer::updateData(const void *data, const unsigned int size, const unsigned int offset) const {
    bind();
    GLCall(glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data));
    unbind();
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

void UniformBuffer::unbind() {
    GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}
