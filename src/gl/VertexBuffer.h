#ifndef VERTEXBUFFER_H
#define VERTEXBUFFER_H

#include "GL/glew.h"

enum class BufferUsage {
    STATIC = GL_STATIC_DRAW,
    DYNAMIC = GL_DYNAMIC_DRAW,
    STREAM = GL_STREAM_DRAW
};

class VertexBuffer {
private:
    unsigned int m_ID = 0;

public:
    VertexBuffer();
    ~VertexBuffer();

    VertexBuffer(const VertexBuffer &other);
    VertexBuffer & operator=(const VertexBuffer &other);

    VertexBuffer(VertexBuffer &&other) noexcept;
    VertexBuffer & operator=(VertexBuffer &&other) noexcept;

    void init(const void *data, unsigned int size, BufferUsage usage = BufferUsage::STATIC);
    void updateData(const void *data, unsigned int size, unsigned int offset = 0) const;
    void deleteBuffer();
    void bind() const;
    static void unbind();
};

#endif //VERTEXBUFFER_H
