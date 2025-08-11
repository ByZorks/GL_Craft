#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"

class VertexArray {
private:
    unsigned int m_ID = 0;
    unsigned int m_nextAttributeIndex = 0;

public:
    VertexArray();
    ~VertexArray();

    void init();
    void addBuffer(const VertexBuffer &vb, const VertexBufferLayout &layout);
    void addInstancedBuffer(const VertexBuffer &vb, unsigned int attributeIndex, unsigned int componentCount = 3) const;
    void deleteBuffer();
    void bind() const;
    static void unbind();
};

#endif //VERTEXARRAY_H
