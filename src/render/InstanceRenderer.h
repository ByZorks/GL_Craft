#ifndef INSTANCERENDERER_H
#define INSTANCERENDERER_H
#include "vec3.hpp"
#include "../gl/IndexBuffer.h"
#include "../gl/VertexArray.h"
#include "../world/Block.h"

class InstanceRenderer {
private:
    VertexArray m_VAO;
    VertexBuffer m_VBO;
    VertexBuffer m_instanceVBO;
    IndexBuffer m_IBO;

    std::vector<glm::vec3> m_instancePositions;
    bool m_buffersInitialized = false;
    unsigned int m_instanceCount = 0;
    size_t m_instanceBufferCapacity = 0;

public:
    virtual ~InstanceRenderer() = default;

    void init();
    void addInstance(const glm::vec3& position);
    void updateInstanceBuffer();
    void resetInstances();
    void draw() const;

    [[nodiscard]] unsigned int m_instance_count() const;

private:
    virtual void addVertices(std::vector<BlockVertex> &vertices);
};

#endif //INSTANCERENDERER_H
