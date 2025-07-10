#ifndef RENDERER_H
#define RENDERER_H
#include "IndexBuffer.h"
#include "VertexArray.h"

class Renderer {
private:
    static float m_deltaTime;
    static float m_lastFrame;

public:
    static void clear();
    static float calculateDeltaTime(float currentFrame);
    static void draw(const VertexArray& vao, const IndexBuffer& ibo);

};

#endif //RENDERER_H
