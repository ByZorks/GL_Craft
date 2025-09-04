#ifndef GL_CRAFT_TEXTUREARRAY_H
#define GL_CRAFT_TEXTUREARRAY_H
#include <string>
#include <vector>

class TextureArray {
public:
    TextureArray(int width, int height, std::string dirPath);
    ~TextureArray();

    void bind(unsigned int slot) const;

private:
    static std::vector<std::string> getFilesInDirectory(const std::string& dirPath);

private:
    unsigned int m_ID = 0;
    int m_width, m_height;
    std::string m_dirPath;
};

#endif //GL_CRAFT_TEXTUREARRAY_H