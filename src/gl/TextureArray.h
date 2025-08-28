#ifndef GL_CRAFT_TEXTUREARRAY_H
#define GL_CRAFT_TEXTUREARRAY_H
#include <string>
#include <vector>

class TextureArray {
    unsigned int m_ID = 0;
    int m_width, m_height, m_layers;
    std::string m_dirPath;

public:
    TextureArray(int width, int height, int layers, std::string  dirPath);
    ~TextureArray();

    void bind(unsigned int slot) const;

private:
    static std::vector<std::string> getFilesInDirectory(const std::string& dirPath);
};

#endif //GL_CRAFT_TEXTUREARRAY_H