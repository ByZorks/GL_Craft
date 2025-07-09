#include "Shader.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <utility>

Shader::Shader(std::string vertexPath, std::string fragmentPath) : m_vertexFilePath(std::move(vertexPath)),
    m_fragmentFilePath(std::move(fragmentPath)) {
}

Shader::~Shader() {
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
    }
}

unsigned int Shader::use() {
    const std::string &vertexShader = readFile(m_vertexFilePath);
    const std::string &fragmentShader = readFile(m_fragmentFilePath);

    if (vertexShader.empty() || fragmentShader.empty()) {
        return 0;
    }

    const unsigned int program = glCreateProgram();
    const unsigned int vs = compile(GL_VERTEX_SHADER, vertexShader);
    const unsigned int fs = compile(GL_FRAGMENT_SHADER, fragmentShader);

    if (vs == 0 || fs == 0) {
        glDeleteProgram(program);
        return 0;
    }

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader error: " << infoLog << std::endl;
        glDeleteProgram(program);
        glDeleteShader(vs);
        glDeleteShader(fs);
        return 0;
    }

    glValidateProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glUseProgram(program);
    m_programId = program;

    return program;
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
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        const auto message = static_cast<char *>(alloca(length * sizeof(char)));
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "Failed to compile " << (shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" <<
                std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);
        return 0;
    }

    return id;
}
