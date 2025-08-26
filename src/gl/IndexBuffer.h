#ifndef INDEXBUFFER_H
#define INDEXBUFFER_H

class IndexBuffer {
private:
    unsigned int m_ID = 0;
    unsigned int m_Count = 0;

public:
    IndexBuffer();
    ~IndexBuffer();

    IndexBuffer(const IndexBuffer &other);
    IndexBuffer & operator=(const IndexBuffer &other);

    IndexBuffer(IndexBuffer &&other) noexcept;
    IndexBuffer & operator=(IndexBuffer &&other) noexcept;

    void init(const unsigned int *data, unsigned int count);
    void updateData(const unsigned int *data) const;
    void deleteBuffer();

    [[nodiscard]] unsigned int getCount() const;

    [[nodiscard]] unsigned int getID() const;
};

#endif //INDEXBUFFER_H
