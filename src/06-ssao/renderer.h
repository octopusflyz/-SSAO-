#pragma once

#include "shader.h"

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    bool init();
    void render(float time);

private:
    unsigned int cubeVAO_ = 0, cubeVBO_ = 0;
    Shader shader_;

    void createCube();
};
