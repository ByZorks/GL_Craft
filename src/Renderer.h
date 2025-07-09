#ifndef RENDERER_H
#define RENDERER_H
#include "IndexBuffer.h"
#include "VertexArray.h"

class Renderer {
public:
    static void Clear();
    static void Draw(const VertexArray& vao, const IndexBuffer& ibo);

};

#endif //RENDERER_H
