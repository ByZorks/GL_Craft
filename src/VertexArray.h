#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H

class VertexArray {
private:
    unsigned int m_RendererID = 0;

public:
    VertexArray();
    ~VertexArray();

    void Bind() const;
    static void Unbind();

};

#endif //VERTEXARRAY_H
