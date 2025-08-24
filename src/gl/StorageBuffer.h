#ifndef GL_CRAFT_STORAGEBUFFER_H
#define GL_CRAFT_STORAGEBUFFER_H
#include <cstddef>

class StorageBuffer {
private:
    unsigned int m_ID = 0;
    unsigned int m_bindingPoint = 999;
    size_t m_size = 0;

public:
    StorageBuffer();
    ~StorageBuffer();

    StorageBuffer(const StorageBuffer &other);
    StorageBuffer(StorageBuffer &&other) noexcept;
    StorageBuffer & operator=(const StorageBuffer &other);
    StorageBuffer & operator=(StorageBuffer &&other) noexcept;

    void init(const void *data, size_t size, unsigned int bindingPoint);
    size_t updateData(const void *data, size_t size, unsigned int offset = 0);
    void deleteBuffer();
    void bind() const;
    static void unbind();
};

#endif //GL_CRAFT_STORAGEBUFFER_H