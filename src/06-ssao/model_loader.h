#pragma once

#include <string>
#include <GL/glew.h>

// Forward declarations to avoid conflicts with tinygltf::Model
namespace tinygltf {
    class Model;
    class Primitive;
}

class Model {
public:
    Model();
    ~Model();

    bool loadFromFile(const std::string &path);
    void draw() const;
    bool isLoaded() const;
    GLuint getDiffuseTex() const;

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
    int indexCount_ = 0;
    bool loaded_ = false;
    GLuint diffuseTex_ = 0;

    void cleanup();
    void loadTexture(const tinygltf::Model& model, const tinygltf::Primitive& prim, const std::string& modelPath);
};

