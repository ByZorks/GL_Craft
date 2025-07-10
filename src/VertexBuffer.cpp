#include "VertexBuffer.h"

#include "OpenGLDebug.h"
#include "GL/glew.h"

VertexBuffer::VertexBuffer() = default;

VertexBuffer::~VertexBuffer() {
    GLCall(glDeleteBuffers(1, &m_RendererID));
}

void VertexBuffer::init(const void *data, const unsigned int size) {
    GLCall(glGenBuffers(1, &m_RendererID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
}

void VertexBuffer::bind() const {
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
}

void VertexBuffer::unbind() {
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}
