#include "../gl/Texture.h"

#include <utility>

#include "glad/gl.h"

#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <ostream>

#include "stb/stb_image.h"

Texture::Texture(std::string filePath) : m_FilePath(std::move(filePath)), m_LocalBuffer(nullptr),
                                         m_Width(0), m_Height(0), m_BPP(0) {
    stbi_set_flip_vertically_on_load(1);

    m_LocalBuffer = stbi_load(m_FilePath.c_str(), &m_Width, &m_Height, &m_BPP, 4); // 4 for RGBA
    if (!m_LocalBuffer) std::cerr << "Cannot load texture: " << m_FilePath << std::endl;

    glCreateTextures(GL_TEXTURE_2D, 1, &m_ID);

    glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTextureStorage2D(m_ID, 1, GL_RGBA8, m_Width, m_Height);
    glTextureSubImage2D(m_ID, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, m_LocalBuffer);
    glGenerateTextureMipmap(m_ID);

    if (m_LocalBuffer) stbi_image_free(m_LocalBuffer);
}

Texture::Texture(const int width, const int height, const bool isDepthTexture) : m_LocalBuffer(nullptr), m_Width(width), m_Height(height), m_BPP(0) {
    glCreateTextures(GL_TEXTURE_2D, 1, &m_ID);

    glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (isDepthTexture) {
        glTextureStorage2D(m_ID, 1, GL_DEPTH_COMPONENT24, m_Width, m_Height);
    } else {
        glTextureStorage2D(m_ID, 1, GL_RGBA8, m_Width, m_Height);
    }
}

Texture::~Texture() {
    if (m_ID != 0) {
        glDeleteTextures(1, &m_ID);
    }
}

void Texture::bind(const unsigned int slot) const {
    glBindTextureUnit(slot, m_ID);
}

unsigned int Texture::getID() const {
    return m_ID;
}

void Texture::setID(const unsigned int m_id) {
    m_ID = m_id;
}
