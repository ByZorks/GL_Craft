#ifndef SHADER_H
#define SHADER_H
#include <string>

#include "GL/glew.h"

class Shader {
private:
    unsigned int m_programId = 0;
    std::string m_vertexFilePath;
    std::string m_fragmentFilePath;

public:
    Shader(std::string  vertexPath, std::string  fragmentPath);

    ~Shader();

    unsigned int use();

private:
    static std::string readFile(const std::string &filePath) ;

    static unsigned int compile(GLenum shaderType, const std::string& shader);
};

#endif //SHADER_H
