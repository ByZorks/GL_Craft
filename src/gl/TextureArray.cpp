#include "TextureArray.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>

#include "glad/gl.h"
#include "stb/stb_image.h"


TextureArray::TextureArray(const int width, const int height, std::string dirPath) : m_width(width), m_height(height),
    m_dirPath(std::move(dirPath)) {
    stbi_set_flip_vertically_on_load(true);

    std::vector<std::string> files = getFilesInDirectory(m_dirPath);
    std::ranges::sort(files, [](const std::string_view &a, const std::string_view &b) {
        return a < b;
    });
    const auto layersCount = static_cast<unsigned int>(files.size());

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_ID);
    const int mipLevels = 1 + static_cast<int>(std::floor(std::log2(std::max(m_width, m_height))));
    glTextureStorage3D(m_ID, mipLevels, GL_RGBA8, m_width, m_height, static_cast<GLsizei>(layersCount));

    glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Anisotropic filtering if supported
    float maxAnisotropicFiltering = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropicFiltering);
    if (maxAnisotropicFiltering > 0.0f) {
        constexpr float anisotropicFiltering = 16.0f;
        glTextureParameterf(m_ID, GL_TEXTURE_MAX_ANISOTROPY, std::min(maxAnisotropicFiltering, anisotropicFiltering));
    }

    for (unsigned int i = 0; i < layersCount; i++) {
        unsigned char *buffer = stbi_load(files[i].c_str(), &m_width, &m_height, nullptr, 4);
        if (!buffer) {
            std::cerr << "Failed to load texture: " << files[i] << std::endl;
            continue;
        }

        // Premultiply alpha
        for (int p = 0; p < m_width * m_height; p++) {
            unsigned char *px = buffer + p * 4;
            const float a = static_cast<float>(px[3]) / 255.0f;
            px[0] = static_cast<unsigned char>(static_cast<float>(px[0]) * a);
            px[1] = static_cast<unsigned char>(static_cast<float>(px[1]) * a);
            px[2] = static_cast<unsigned char>(static_cast<float>(px[2]) * a);
        }

        glTextureSubImage3D(m_ID, 0, 0, 0, static_cast<GLint>(i), m_width, m_height, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                            buffer);

        stbi_image_free(buffer);
    }

    glGenerateTextureMipmap(m_ID);
}

TextureArray::~TextureArray() {
    if (m_ID != 0) {
        glDeleteTextures(1, &m_ID);
    }
}

void TextureArray::bind(const unsigned int slot) const {
    glBindTextureUnit(slot, m_ID);
}

std::vector<std::string> TextureArray::getFilesInDirectory(const std::string &dirPath) {
    std::vector<std::string> files;
    for (const auto &entry: std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().string());
        }
    }
    return files;
}
