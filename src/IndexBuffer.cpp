#include "IndexBuffer.h"

#include "OpenGLDebug.h"
#include "GL/glew.h"

IndexBuffer::IndexBuffer() = default;

IndexBuffer::~IndexBuffer() {
    GLCall(glDeleteBuffers(1, &m_rendererID));
}

void IndexBuffer::init(const unsigned int *data, const unsigned int count) {
    m_Count = count;
    GLCall(glGenBuffers(1, &m_rendererID));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_STATIC_DRAW));
}

void IndexBuffer::bind() const {
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID));
}

void IndexBuffer::unbind() {
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

unsigned int IndexBuffer::m_count() const {
    return m_Count;
}
