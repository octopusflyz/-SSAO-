#include "shader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

Shader::Shader(const std::string &vertPath, const std::string &fragPath) {
    loadFromFiles(vertPath, fragPath);
}

std::string Shader::readFile(const std::string &path) {
    // Try the provided path first
    std::ifstream in(path);
    if (in) {
        std::ostringstream ss; ss << in.rdbuf(); return ss.str();
    }

    // Try common relative locations from the executable's working directory
    std::vector<std::string> candidates;
    candidates.push_back(std::string("shaders/") + path);
    candidates.push_back(std::string("../shaders/") + path);
    candidates.push_back(std::string("../../shaders/") + path);

#ifdef SHADER_SOURCE_DIR
    // Fallback to the source shaders directory embedded at compile time (useful in development)
    std::string filename = path.substr(path.find_last_of("/\\") + 1);
    candidates.push_back(std::string(SHADER_SOURCE_DIR) + "/" + filename);
#endif

    for (auto &c : candidates) {
        std::ifstream tryin(c);
        if (tryin) {
            std::ostringstream ss; ss << tryin.rdbuf();
            std::cerr << "Loaded shader fallback: " << c << std::endl;
            return ss.str();
        }
    }

    std::cerr << "Failed to read shader file: " << path << " (tried " << candidates.size() << " fallback paths)" << std::endl;
    return std::string();
}

GLuint Shader::compileShader(const std::string &src, GLenum type) {
    const char *s = src.c_str();
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &s, nullptr);
    glCompileShader(id);
    GLint ok = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(id, len, nullptr, &log[0]);
        std::cerr << "Shader compile error: " << log << std::endl;
        glDeleteShader(id);
        return 0;
    }
    return id;
}

bool Shader::loadFromFiles(const std::string &vertPath, const std::string &fragPath) {
    std::string vsrc = readFile(vertPath);
    std::string fsrc = readFile(fragPath);
    if (vsrc.empty() || fsrc.empty()) {
        std::cerr << "Failed to read shader files: " << vertPath << ", " << fragPath << std::endl;
        return false;
    }
    GLuint vs = compileShader(vsrc, GL_VERTEX_SHADER);
    GLuint fs = compileShader(fsrc, GL_FRAGMENT_SHADER);
    if (!vs || !fs) return false;
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(prog, len, nullptr, &log[0]);
        std::cerr << "Program link error: " << log << std::endl;
        glDeleteProgram(prog);
        return false;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    program_ = prog;
    return true;
}

void Shader::use() const { if (program_) glUseProgram(program_); }

Shader::~Shader() { if (program_) glDeleteProgram(program_); }
