#include "TextureArray.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>

#include "OpenGLDebug.h"
#include "glad/gl.h"
#include "stb/stb_image.h"


TextureArray::TextureArray(const int width, const int height, std::string dirPath) : m_width(width), m_height(height), m_dirPath(std::move(dirPath)) {
    stbi_set_flip_vertically_on_load(true);

    std::vector<std::string> files = getFilesInDirectory(m_dirPath);
    std::ranges::sort(files, [](const std::string &a, const std::string &b) {
        return a < b;
    });
    const unsigned int layersCount = files.size();

    GLCall(glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_ID));
    const int mipLevels = 1 + static_cast<int>(std::floor(std::log2(std::max(m_width, m_height))));
    GLCall(glTextureStorage3D(m_ID, mipLevels, GL_RGBA8, m_width, m_height, layersCount));

    GLCall(glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR));
    GLCall(glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GLCall(glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    // Anisotropic filtering if supported
    float maxAnisotropicFiltering = 0.0f;
    GLCall(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropicFiltering));
    if (maxAnisotropicFiltering > 0.0f) {
        constexpr float anisotropicFiltering = 16.0f;
        GLCall(glTextureParameterf(m_ID, GL_TEXTURE_MAX_ANISOTROPY, std::min(maxAnisotropicFiltering, anisotropicFiltering)));
    }

    for (unsigned int i = 0; i < layersCount; i++) {
        unsigned char* buffer = stbi_load(files[i].c_str(), &m_width, &m_height, nullptr, 4);
        if (!buffer) {
            std::cerr << "Failed to load texture: " << files[i] << std::endl;
            continue;
        }

        // Premultiply alpha
        for (int p = 0; p < m_width * m_height; p++) {
            unsigned char* px = buffer + p * 4;
            const float a = static_cast<float>(px[3]) / 255.0f;
            px[0] = static_cast<unsigned char>(static_cast<float>(px[0]) * a);
            px[1] = static_cast<unsigned char>(static_cast<float>(px[1]) * a);
            px[2] = static_cast<unsigned char>(static_cast<float>(px[2]) * a);
        }

        GLCall(glTextureSubImage3D(m_ID, 0, 0, 0, i, m_width, m_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer));
    }

    GLCall(glGenerateTextureMipmap(m_ID));
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
