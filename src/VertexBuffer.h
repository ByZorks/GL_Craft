#ifndef VERTEXBUFFER_H
#define VERTEXBUFFER_H

class VertexBuffer {
private:
    unsigned int m_RendererID = 0;

public:
    VertexBuffer();
    ~VertexBuffer();

    void init(const void *data, unsigned int size);
    void bind() const;
    static void unbind() ;

};

#endif //VERTEXBUFFER_H
