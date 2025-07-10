#ifndef INDEXBUFFER_H
#define INDEXBUFFER_H

class IndexBuffer {
private:
    unsigned int m_rendererID = 0;
    unsigned int m_Count = 0;

public:
    IndexBuffer();
    ~IndexBuffer();

    void init(const unsigned int *data, unsigned int count);
    void bind() const;
    static void unbind();

    [[nodiscard]] unsigned int m_count() const;
};

#endif //INDEXBUFFER_H
