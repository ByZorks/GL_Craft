#ifndef SHADER_H
#define SHADER_H
#include <string>
#include <unordered_map>

#include "glad/gl.h"
#include "glm/fwd.hpp"

class Shader {
public:
    Shader(std::string vertexPath, std::string fragmentPath);
    ~Shader();

    void use() const;
    int getUniformLocation(const std::string &name);
    void setUniform1i(const std::string &name, int value);
    void setUniform1f(const std::string &name, float value);
    void setUniform1b(const std::string &name, bool value);
    void setUniform3f(const std::string &name, float v0, float v1, float v2);
    void setUniformMat4f(const std::string &name, const glm::mat4 &matrix);

private:
    static std::string readFile(const std::string &filePath);
    static unsigned int compile(GLenum shaderType, const std::string& shader);
    void compileAndLink();

private:
    struct TransparentStringHash {
        using is_transparent = void;
        size_t operator()(const std::string_view txt) const noexcept {
            return std::hash<std::string_view>{}(txt);
        }
        size_t operator()(const std::string& txt) const noexcept {
            return std::hash<std::string_view>{}(txt);
        }
        size_t operator()(const char* txt) const noexcept {
            return std::hash<std::string_view>{}(txt);
        }
    };

    unsigned int m_ID = 0;
    std::string m_vertexFilePath, m_fragmentFilePath;
    std::unordered_map<std::string, int, TransparentStringHash, std::equal_to<>> m_uniformLocationCache;
    bool m_isInitialized = false;
};

#endif //SHADER_H
