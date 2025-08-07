#include "../gl/Texture.h"

#include <utility>

#include "../core/OpenGLDebug.h"
#include "GL/glew.h"

#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <ostream>

#include "stb_image.h"

Texture::Texture(std::string filePath) : m_FilePath(std::move(filePath)), m_LocalBuffer(nullptr),
                                         m_Width(0), m_Height(0), m_BPP(0) {
    stbi_set_flip_vertically_on_load(1);

    m_LocalBuffer = stbi_load(m_FilePath.c_str(), &m_Width, &m_Height, &m_BPP, 4); // 4 for RGBA
    if (!m_LocalBuffer) std::cerr << "Cannot load texture: " << m_FilePath << std::endl;

    GLCall(glGenTextures(1, &m_RendererID));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));

    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LocalBuffer));

    if (m_LocalBuffer) stbi_image_free(m_LocalBuffer);
}

Texture::Texture(const int width, const int height, const bool isDepthTexture) : m_LocalBuffer(nullptr), m_Width(width), m_Height(height), m_BPP(0) {
    GLCall(glGenTextures(1, &m_RendererID));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));

    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    if (isDepthTexture) {
        GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));
    } else {
        GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
    }
}

Texture::~Texture() {
    if (m_RendererID != 0) {
        GLCall(glDeleteTextures(1, &m_RendererID));
    }
}

void Texture::bind(const unsigned int slot) const {
    GLCall(glActiveTexture(GL_TEXTURE0 + slot));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));
}

void Texture::unbind() {
    GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

unsigned int Texture::m_renderer_id() const {
    return m_RendererID;
}

void Texture::set_m_renderer_id(const unsigned int m_renderer_id) {
    m_RendererID = m_renderer_id;
}
