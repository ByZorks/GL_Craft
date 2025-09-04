#ifndef GL_CRAFT_INDIRECTBUFFER_H
#define GL_CRAFT_INDIRECTBUFFER_H

class IndirectBuffer {
public:
    IndirectBuffer();
    ~IndirectBuffer();

    void init(const void *data, unsigned int size);
    void updateData(const void *data, unsigned int size, unsigned int offset = 0);
    void resize(unsigned int newSize);
    void deleteBuffer();
    void bind() const;

private:
    unsigned int m_ID = 0;
    unsigned int m_size = 0;
};

#endif //GL_CRAFT_INDIRECTBUFFER_H