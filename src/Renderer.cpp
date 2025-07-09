#include "Renderer.h"

#include "OpenGLDebug.h"
#include "GL/glew.h"

void Renderer::Clear() {
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::Draw(const VertexArray& vao, const IndexBuffer& ibo) {
    vao.Bind();
    ibo.Bind();
    GLCall(glDrawElements(GL_TRIANGLES, ibo.m_count(), GL_UNSIGNED_INT, nullptr));
}
