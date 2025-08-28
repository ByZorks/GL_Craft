#include "TextureArray.h"

#include <utility>
#include <filesystem>
#include <iostream>
#include <vector>

#include "OpenGLDebug.h"
#include "glad/gl.h"
#include "stb/stb_image.h"


TextureArray::TextureArray(const int width, const int height, const int layers, std::string dirPath) : m_width(width), m_height(height), m_layers(layers), m_dirPath(std::move(dirPath)) {
    stbi_set_flip_vertically_on_load(true);

    const std::vector<std::string> files = getFilesInDirectory(m_dirPath);
    if (files.size() != static_cast<size_t>(m_layers)) {
        std::cerr << "Number of files in directory (" << files.size() <<
                ") does not match the specified number of layers (" << m_layers << ")" << std::endl;
        return;
    }

    GLCall(glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_ID));
    GLCall(glTextureStorage3D(m_ID, 1, GL_RGBA8, m_width, m_height, m_layers));

    GLCall(glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GLCall(glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GLCall(glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    for (unsigned int i = 0; i < m_layers; i++) {
        const unsigned char* buffer = stbi_load(files[i].c_str(), &m_width, &m_height, nullptr, 4);
        if (!buffer) {
            std::cerr << "Failed to load texture: " << files[i] << std::endl;
            continue;
        }
        GLCall(glTextureSubImage3D(m_ID, 0, 0, 0, i, m_width, m_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer));
    }
}

TextureArray::~TextureArray() {
    if (m_ID != 0) {
        GLCall(glDeleteTextures(1, &m_ID));
    }
}

void TextureArray::bind(const unsigned int slot) const {
    GLCall(glBindTextureUnit(slot, m_ID));
}

std::vector<std::string> TextureArray::getFilesInDirectory(const std::string &dirPath) {
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().string());
        }
    }
    return files;
}
