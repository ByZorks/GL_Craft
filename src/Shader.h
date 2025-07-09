#ifndef SHADER_H
#define SHADER_H
#include <string>
#include <unordered_map>

#include "GL/glew.h"

class Shader {
private:
    unsigned int m_programId = 0;
    std::string m_vertexFilePath;
    std::string m_fragmentFilePath;
    std::pmr::unordered_map<std::string, int> m_uniformLocationCache;

public:
    Shader(std::string  vertexPath, std::string  fragmentPath);
    ~Shader();

    unsigned int use();
    int getUniformLocation(const std::string &name);
    void setUniform4f(const std::string &name, float v0, float v1, float v2, float v3);

    [[nodiscard]] unsigned int m_program_id() const;

private:
    static std::string readFile(const std::string &filePath);

    static unsigned int compile(GLenum shaderType, const std::string& shader);
};

#endif //SHADER_H
