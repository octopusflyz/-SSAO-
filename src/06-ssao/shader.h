#pragma once

#include <string>
#include <GL/glew.h>

class Shader {
public:
    Shader() = default;
    explicit Shader(const std::string &vertPath, const std::string &fragPath);
    ~Shader();

    bool loadFromFiles(const std::string &vertPath, const std::string &fragPath);
    void use() const;
    GLuint id() const { return program_; }

private:
    GLuint program_ = 0;
    GLuint compileShader(const std::string &src, GLenum type);
    std::string readFile(const std::string &path);

    // helper for printing compile errors in a clearer way
    void printCompileError(GLuint id, GLenum type);
};
