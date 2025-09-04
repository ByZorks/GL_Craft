#ifndef RENDERER_H
#define RENDERER_H
#include "../gl/IndexBuffer.h"
#include "../gl/IndirectBuffer.h"
#include "../gl/StorageBuffer.h"
#include "../gl/VertexArray.h"

class Renderer {
public:
    static float s_renderDistance;

    static void init();
    static void clear();

    static void disableWireFrameMode();
    static void enableWireFrameMode();
    static void disableDepthTesting();
    static void enableDepthTesting();
    static void disableDepthMask();
    static void enableDepthMask();
    static void disableBackFaceCulling();
    static void enableBackFaceCulling();

    static void drawLines(const VertexArray &vao, unsigned int IBOCount);
    static void drawWithVertexPulling(const VertexArray &vao, const StorageBuffer &ssbo, unsigned int vertexCount);
    static void drawWithVertexPullingInstanced(const VertexArray &vao, const StorageBuffer &ssbo,
                                               const StorageBuffer &instanceSsbo, unsigned int vertexCount,
                                               unsigned int instanceCount);
    static void drawMultiWithVertexPulling(const IndirectBuffer &cmds, const StorageBuffer &vertices,
                                           const StorageBuffer &offsets, unsigned int drawCount, const void *offset);
    static void drawElements(const VertexArray &vao, unsigned int IBOCount);
    static void drawElementsInstanced(const VertexArray &vao, unsigned int IBOCount, unsigned int instanceCount);
};

#endif //RENDERER_H
