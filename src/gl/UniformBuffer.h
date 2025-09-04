#ifndef GL_CRAFT_UNIFORMBUFFER_H
#define GL_CRAFT_UNIFORMBUFFER_H

class UniformBuffer {
public:
    UniformBuffer();
    ~UniformBuffer();

    void init(const void *data, unsigned int size, unsigned int bindingPoint);
    void updateData(const void *data, unsigned int size, unsigned int offset = 0) const;
    void deleteBuffer();

private:
    unsigned int m_ID = 0;
    unsigned int m_bindingPoint = 0;

};

#endif //GL_CRAFT_UNIFORMBUFFER_H