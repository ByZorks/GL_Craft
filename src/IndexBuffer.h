#ifndef INDEXBUFFER_H
#define INDEXBUFFER_H

class IndexBuffer {
private:
    unsigned int m_rendererID = 0;
    unsigned int m_Count;

public:
    IndexBuffer(const unsigned int *data, unsigned int count);
    ~IndexBuffer();

    void Bind() const;
    static void Unbind();

    [[nodiscard]] unsigned int m_count() const;
};

#endif //INDEXBUFFER_H
