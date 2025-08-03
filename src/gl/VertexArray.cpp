#include "VertexArray.h"

#include "../core/OpenGLDebug.h"
#include "GL/glew.h"

VertexArray::VertexArray() = default;

VertexArray::~VertexArray() {
    if (m_RendererID != 0) {
        GLCall(glDeleteVertexArrays(1, &m_RendererID));
    }
}

void VertexArray::init() {
    GLCall(glGenVertexArrays(1, &m_RendererID));
    GLCall(glBindVertexArray(m_RendererID));
}

void VertexArray::AddBuffer(const VertexBuffer &vb, const VertexBufferLayout &layout) {
    bind();
    vb.bind();
    const auto& elements = layout.m_elements();
    unsigned int offset = 0;
    for (unsigned int i = 0; i < elements.size(); i++) {
        const auto&[type, count, normalized, isInteger] = elements[i];
        GLCall(glEnableVertexAttribArray(m_nextAttributeIndex + i));
        if (isInteger) {
            GLCall(glVertexAttribIPointer(m_nextAttributeIndex + i, count, type, layout.m_stride(), reinterpret_cast<const void *>(offset)));
        } else {
            GLCall(glVertexAttribPointer(m_nextAttributeIndex + i, count, type, normalized, layout.m_stride(), reinterpret_cast<const void *>(offset)));
        }
        offset += count * VertexBufferElement::GetSizeOfType(type);
    }
    m_nextAttributeIndex += elements.size();
}

void VertexArray::addInstancedBuffer(const VertexBuffer &vb, const unsigned int attributeIndex, const unsigned int componentCount) const {
    bind();
    vb.bind();
    GLCall(glEnableVertexAttribArray(attributeIndex));
    GLCall(glVertexAttribPointer(attributeIndex, componentCount, GL_FLOAT, GL_FALSE, componentCount * sizeof(float), nullptr));
    GLCall(glVertexAttribDivisor(attributeIndex, 1));
}

void VertexArray::bind() const {
    GLCall(glBindVertexArray(m_RendererID));
}

void VertexArray::unbind() {
    GLCall(glBindVertexArray(0));
}
