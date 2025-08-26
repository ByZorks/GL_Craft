#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"

class VertexArray {
private:
    unsigned int m_ID = 0;
    unsigned int m_nextAttributeIndex = 0;
    unsigned int m_nextBindingIndex = 0;

public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray &other);
    VertexArray & operator=(const VertexArray &other);

    VertexArray(VertexArray &&other) noexcept;
    VertexArray & operator=(VertexArray &&other) noexcept;

    void init();
    void addBuffer(const VertexBuffer &vb, const IndexBuffer &ibo, const VertexBufferLayout &layout);
    void addInstancedBuffer(const VertexBuffer &vb, unsigned int attributeIndex, unsigned int componentCount = 3);
    void deleteBuffer();
    void bind() const;
};

#endif //VERTEXARRAY_H
