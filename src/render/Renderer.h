#ifndef RENDERER_H
#define RENDERER_H
#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"

class Renderer {
private:
    static float m_deltaTime;
    static float m_lastFrame;

public:
    static float s_renderDistance;

    static void init();
    static void clear();
    static float calculateDeltaTime(float currentFrame);
    static void disableWireFrameMode();
    static void enableWireFrameMode();
    static void disableDepthTesting();
    static void enableDepthTesting();
    static void disableDepthMask();
    static void enableDepthMask();
    static void disableBackFaceCulling();
    static void enableBackFaceCulling();
    static void draw(const VertexArray& vao, const IndexBuffer& ibo);
    static void drawLines(const VertexArray& vao, const IndexBuffer& ibo);
    static void drawInstanced(const VertexArray& vao, const IndexBuffer& ibo, unsigned int instanceCount);

};

#endif //RENDERER_H
