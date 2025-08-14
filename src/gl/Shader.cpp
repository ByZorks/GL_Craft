#include "Shader.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <utility>

#include "../core/OpenGLDebug.h"
#include "glm.hpp"

Shader::Shader(std::string vertexPath, std::string fragmentPath) : m_vertexFilePath(std::move(vertexPath)),
                                                                   m_fragmentFilePath(std::move(fragmentPath)) {
    compileAndLink();
}

Shader::~Shader() {
    if (m_ID != 0) {
        GLCall(glDeleteProgram(m_ID));
    }
}

void Shader::use() const {
    if (m_isInitialized) {
        GLCall(glUseProgram(m_ID));
    }
}

int Shader::getUniformLocation(const std::string &name) {
    if (m_uniformLocationCache.contains(name)) return m_uniformLocationCache[name];

    GLCall(const int location = glGetUniformLocation(m_ID, name.c_str()));
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name << "' doesn't exist or is not used in the shader." << std::endl;
    }

    m_uniformLocationCache[name] = location;
    return location;
}

void Shader::setUniform1i(const std::string &name, const int value) {
    GLCall(glUniform1i(getUniformLocation(name), value));
}

void Shader::setUniform1f(const std::string &name, const float value) {
    GLCall(glUniform1f(getUniformLocation(name), value));
}

void Shader::setUniform1b(const std::string &name, const bool value) {
    GLCall(glUniform1i(getUniformLocation(name), value));
}

void Shader::setUniform3f(const std::string &name, const float v0, const float v1, const float v2) {
    GLCall(glUniform3f(getUniformLocation(name), v0, v1, v2));
}

void Shader::setUniformMat4f(const std::string &name, const glm::mat4 &matrix) {
    GLCall(glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &matrix[0][0]));
}

std::string Shader::readFile(const std::string &filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int Shader::compile(const GLenum shaderType, const std::string &shader) {
    const unsigned int id = glCreateShader(shaderType);
    const char *src = shader.c_str();
    GLCall(glShaderSource(id, 1, &src, nullptr));
    GLCall(glCompileShader(id));

    int result;
    GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
    if (result == GL_FALSE) {
        int length;
        GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
        const auto message = static_cast<char *>(alloca(length * sizeof(char)));
        GLCall(glGetShaderInfoLog(id, length, &length, message));
        std::cout << "Failed to compile " << (shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" <<
                std::endl;
        std::cout << message << std::endl;
        GLCall(glDeleteShader(id));
        return 0;
    }

    return id;
}

void Shader::compileAndLink() {
    const std::string &vertexShader = readFile(m_vertexFilePath);
    const std::string &fragmentShader = readFile(m_fragmentFilePath);

    if (vertexShader.empty() || fragmentShader.empty()) {
        return;
    }

    const unsigned int program = glCreateProgram();
    const unsigned int vs = compile(GL_VERTEX_SHADER, vertexShader);
    const unsigned int fs = compile(GL_FRAGMENT_SHADER, fragmentShader);

    if (vs == 0 || fs == 0) {
        if (program != 0) GLCall(glDeleteProgram(program));
        if (vs != 0) GLCall(glDeleteShader(vs));
        if (fs != 0) GLCall(glDeleteShader(fs));
        return;
    }

    GLCall(glAttachShader(program, vs));
    GLCall(glAttachShader(program, fs));
    GLCall(glLinkProgram(program));

    int success;
    GLCall(glGetProgramiv(program, GL_LINK_STATUS, &success));
    if (!success) {
        char infoLog[512];
        GLCall(glGetProgramInfoLog(program, 512, nullptr, infoLog));
        std::cerr << "Shader error: " << infoLog << std::endl;
        GLCall(glDeleteProgram(program));
        GLCall(glDeleteShader(vs));
        GLCall(glDeleteShader(fs));
        return;
    }

    GLCall(glValidateProgram(program));
    GLCall(glDeleteShader(vs));
    GLCall(glDeleteShader(fs));

    m_ID = program;
    m_isInitialized = true;
}