#include "VertexArray.h"

#include "OpenGLDebug.h"
#include "GL/glew.h"

VertexArray::VertexArray() {
    GLCall(glGenVertexArrays(1, &m_RendererID));
    GLCall(glBindVertexArray(m_RendererID));
}

VertexArray::~VertexArray() {
    GLCall(glDeleteVertexArrays(1, &m_RendererID));
}

void VertexArray::AddBuffer(const VertexBuffer &vb, const VertexBufferLayout &layout) const {
    bind();
    vb.bind();
    const auto& elements = layout.m_elements();
    unsigned int offset = 0;
    for (unsigned int i = 0; i < elements.size(); i++) {
        const auto&[type, count, normalized] = elements[i];
        GLCall(glEnableVertexAttribArray(i));
        GLCall(glVertexAttribPointer(i, count, type, normalized, layout.m_stride(), reinterpret_cast<const void *>(offset)));
        offset += count * VertexBufferElement::GetSizeOfType(type);
    }
}

void VertexArray::bind() const {
    GLCall(glBindVertexArray(m_RendererID));
}

void VertexArray::unbind() {
    GLCall(glBindVertexArray(0));
}
