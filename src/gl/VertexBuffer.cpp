#include "VertexBuffer.h"

#include "../core/OpenGLDebug.h"
#include "GL/glew.h"

VertexBuffer::VertexBuffer() = default;

VertexBuffer::~VertexBuffer() {
    if (m_RendererID != 0) {
        GLCall(glDeleteBuffers(1, &m_RendererID));
    }
}

void VertexBuffer::init(const void *data, const unsigned int size, BufferUsage usage) {
    GLCall(glGenBuffers(1, &m_RendererID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, static_cast<GLenum>(usage)));
}

void VertexBuffer::updateData(const void *data, const unsigned int size, const unsigned int offset) const {
    bind();
    GLCall(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
}

void VertexBuffer::bind() const {
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
}

void VertexBuffer::unbind() {
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}
