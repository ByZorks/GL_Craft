#include "VertexArray.h"

#include "OpenGLDebug.h"
#include "GL/glew.h"

VertexArray::VertexArray() = default;

VertexArray::~VertexArray() {
    if (m_ID != 0) {
        GLCall(glDeleteVertexArrays(1, &m_ID));
    }
}

VertexArray::VertexArray(const VertexArray &other) = default;

VertexArray & VertexArray::operator=(const VertexArray &other) {
    if (this == &other)
        return *this;
    m_ID = other.m_ID;
    m_nextAttributeIndex = other.m_nextAttributeIndex;
    return *this;
}

VertexArray::VertexArray(VertexArray &&other) noexcept
        : m_ID(other.m_ID),
          m_nextAttributeIndex(other.m_nextAttributeIndex) {
    other.m_ID = 0;
    other.m_nextAttributeIndex = 0;
}

VertexArray & VertexArray::operator=(VertexArray &&other) noexcept {
    if (this == &other)
        return *this;
    m_ID = other.m_ID;
    other.m_ID = 0;
    m_nextAttributeIndex = other.m_nextAttributeIndex;
    other.m_nextAttributeIndex = 0;
    return *this;
}

void VertexArray::init() {
    GLCall(glGenVertexArrays(1, &m_ID));
    GLCall(glBindVertexArray(m_ID));
}

void VertexArray::addBuffer(const VertexBuffer &vb, const VertexBufferLayout &layout) {
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
    GLCall(glVertexAttribIPointer(attributeIndex, componentCount, GL_INT, componentCount * sizeof(int), nullptr));
    GLCall(glVertexAttribDivisor(attributeIndex, 1));
}

void VertexArray::deleteBuffer() {
    if (m_ID != 0) {
        GLCall(glDeleteVertexArrays(1, &m_ID));
        m_ID = 0;
        m_nextAttributeIndex = 0;
    }
}

void VertexArray::bind() const {
    GLCall(glBindVertexArray(m_ID));
}

void VertexArray::unbind() {
    GLCall(glBindVertexArray(0));
}
