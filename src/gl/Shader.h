#ifndef SHADER_H
#define SHADER_H
#include <string>
#include <unordered_map>

#include "fwd.hpp"
#include "GL/glew.h"

class Shader {
private:
    unsigned int m_programId = 0;
    std::string m_vertexFilePath;
    std::string m_fragmentFilePath;
    std::pmr::unordered_map<std::string, int> m_uniformLocationCache;
    bool m_isInitialized = false;

public:
    Shader(std::string vertexPath, std::string fragmentPath);
    ~Shader();

    void use() const;
    int getUniformLocation(const std::string &name);
    void setUniform1i(const std::string &name, int value);
    void setUniform3f(const std::string &name, float v0, float v1, float v2);
    void setUniformMat4f(const std::string &name, const glm::mat4 &matrix);

    [[nodiscard]] unsigned int m_program_id() const;

private:
    static std::string readFile(const std::string &filePath);
    static unsigned int compile(GLenum shaderType, const std::string& shader);
    void compileAndLink();
};

#endif //SHADER_H
