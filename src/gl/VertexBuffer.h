#ifndef VERTEXBUFFER_H
#define VERTEXBUFFER_H

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

    void init(const void *data, unsigned int size);
    void updateData(const void *data, unsigned int size, unsigned int offset = 0) const;
    void deleteBuffer();

    [[nodiscard]] unsigned int getID() const;
};

#endif //VERTEXBUFFER_H
