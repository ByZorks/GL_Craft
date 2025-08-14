#ifndef TEXTURE_H
#define TEXTURE_H
#include <string>

class Texture {
private:
    unsigned int m_ID = 0;
    std::string m_FilePath;
    unsigned char *m_LocalBuffer;
    int m_Width, m_Height, m_BPP; // BPP: Bytes Per Pixel

public:
    explicit Texture(std::string filePath);
    Texture(int width, int height, bool isDepthTexture = false);
    ~Texture();

    void bind(unsigned int slot = 0) const;
    static void unbind();

    [[nodiscard]] unsigned int getID() const;
    void setID(unsigned int m_id);
};

#endif //TEXTURE_H
