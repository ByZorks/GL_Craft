#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"

class VertexArray {
private:
    unsigned int m_RendererID = 0;

public:
    VertexArray();
    ~VertexArray();

    void init();
    void AddBuffer(const VertexBuffer &vb, const VertexBufferLayout &layout) const;
    void bind() const;
    static void unbind();

};

#endif //VERTEXARRAY_H
