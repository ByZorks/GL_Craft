#ifndef GL_CRAFT_STORAGEBUFFER_H
#define GL_CRAFT_STORAGEBUFFER_H

class StorageBuffer {
private:
    unsigned int m_ID = 0;
    unsigned int m_bindingPoint = 999;
    unsigned int m_size = 0;

public:
    StorageBuffer();
    ~StorageBuffer();

    StorageBuffer(const StorageBuffer &other);
    StorageBuffer(StorageBuffer &&other) noexcept;
    StorageBuffer & operator=(const StorageBuffer &other);
    StorageBuffer & operator=(StorageBuffer &&other) noexcept;

    void init(const void *data, unsigned int size, unsigned int bindingPoint);
    void updateData(const void *data, unsigned int size, unsigned int offset = 0);
    void deleteBuffer();
    void bind() const;
    static void unbind();

    [[nodiscard]] unsigned int getBindingPoint() const;
};

#endif //GL_CRAFT_STORAGEBUFFER_H