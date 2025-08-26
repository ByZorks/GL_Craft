#include "VertexArray.h"

#include "OpenGLDebug.h"

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
    if (m_ID == 0) {
        GLCall(glCreateVertexArrays(1, &m_ID));
    }
}

void VertexArray::addBuffer(const VertexBuffer &vb, const IndexBuffer &ibo, const VertexBufferLayout &layout) {
    const auto& elements = layout.m_elements();
    unsigned int offset = 0;

    const unsigned int bindingIndex = m_nextBindingIndex;
    m_nextBindingIndex++;
    GLCall(glVertexArrayVertexBuffer(m_ID, bindingIndex, vb.getID(), 0, layout.m_stride()));
    GLCall(glVertexArrayElementBuffer(m_ID, ibo.getID()));

    for (unsigned int i = 0; i < elements.size(); i++) {
        const auto&[type, count, normalized, isInteger] = elements[i];

        GLCall(glEnableVertexArrayAttrib(m_ID, m_nextAttributeIndex + i));
        if (isInteger) {
            GLCall(glVertexArrayAttribIFormat(m_ID, m_nextAttributeIndex + i, count, type, offset));
        } else {
            GLCall(glVertexArrayAttribFormat(m_ID, m_nextAttributeIndex + i, count, type, normalized, offset));
        }

        GLCall(glVertexArrayAttribBinding(m_ID, m_nextAttributeIndex + i, bindingIndex));
        offset += count * VertexBufferElement::GetSizeOfType(type);
    }
    m_nextAttributeIndex += elements.size();
}

void VertexArray::addInstancedBuffer(const VertexBuffer &vb, const unsigned int attributeIndex, const unsigned int componentCount) {
    const unsigned int bindingIndex = m_nextBindingIndex;
    m_nextBindingIndex++;
    GLCall(glVertexArrayVertexBuffer(m_ID, bindingIndex, vb.getID(), 0, componentCount * sizeof(int)));
    GLCall(glEnableVertexArrayAttrib(m_ID, attributeIndex));
    GLCall(glVertexArrayAttribIFormat(m_ID, attributeIndex, componentCount, GL_INT, 0));
    GLCall(glVertexArrayAttribBinding(m_ID, attributeIndex, bindingIndex));
    GLCall(glVertexArrayBindingDivisor(m_ID, attributeIndex, 1));
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
