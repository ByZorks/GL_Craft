#ifndef VERTEXBUFFERLAYOUT_H
#define VERTEXBUFFERLAYOUT_H

#include <stdexcept>
#include <vector>

#include "glad/gl.h"

struct VertexBufferElement {
    unsigned int type;
    unsigned int count;
    unsigned char normalized;
    bool isInteger;

    static unsigned int GetSizeOfType(const unsigned int type) {
        switch (type) {
            case GL_FLOAT:
                return sizeof(GLfloat);
            case GL_UNSIGNED_INT:
                return sizeof(GLuint);
            case GL_UNSIGNED_BYTE:
                return sizeof(GLubyte);
            case GL_BYTE:
                return sizeof(GLbyte);
            default:
                return 0;
        }
    }
};

class VertexBufferLayout {
private:
    std::vector<VertexBufferElement> m_Elements;
    unsigned int m_Stride;

public:
    VertexBufferLayout() : m_Stride(0) {}

    template<typename T>
    void Push(unsigned int count, bool normalized = false) {
        throw std::runtime_error("Unsupported type for VertexBufferLayout::Push");
    }

    template<typename T>
    void PushInt(unsigned int count, bool normalized = false) {
        throw std::runtime_error("Unsupported type for VertexBufferLayout::PushInt");
    }

    [[nodiscard]] std::vector<VertexBufferElement> m_elements() const {
        return m_Elements;
    }

    [[nodiscard]] unsigned int m_stride() const {
        return m_Stride;
    }
};

template<>
inline void VertexBufferLayout::Push<float>(const unsigned int count, const bool normalized) {
    m_Elements.push_back({GL_FLOAT, count, static_cast<unsigned char>(normalized), false});
    m_Stride += VertexBufferElement::GetSizeOfType(GL_FLOAT) * count;
}

template<>
inline void VertexBufferLayout::Push<unsigned int>(const unsigned int count, const bool normalized) {
    m_Elements.push_back({GL_UNSIGNED_INT, count, static_cast<unsigned char>(normalized), false});
    m_Stride += VertexBufferElement::GetSizeOfType(GL_UNSIGNED_INT) * count;
}

template<>
inline void VertexBufferLayout::Push<unsigned char>(const unsigned int count, const bool normalized) {
    m_Elements.push_back({GL_UNSIGNED_BYTE, count, static_cast<unsigned char>(normalized), false});
    m_Stride += VertexBufferElement::GetSizeOfType(GL_UNSIGNED_BYTE) * count;
}

template<>
inline void VertexBufferLayout::Push<char>(const unsigned int count, const bool normalized) {
    m_Elements.push_back({GL_BYTE, count, static_cast<unsigned char>(normalized), false});
    m_Stride += VertexBufferElement::GetSizeOfType(GL_BYTE) * count;
}

template<>
inline void VertexBufferLayout::PushInt<unsigned char>(const unsigned int count, const bool normalized) {
    m_Elements.push_back({GL_UNSIGNED_BYTE, count, static_cast<unsigned char>(normalized), true});
    m_Stride += VertexBufferElement::GetSizeOfType(GL_UNSIGNED_BYTE) * count;
}

#endif //VERTEXBUFFERLAYOUT_H
