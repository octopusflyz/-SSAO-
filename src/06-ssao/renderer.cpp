#include "renderer.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

bool Renderer::init() {
    // load shader (copied to build/shaders by CMake post build)
    if (!shader_.loadFromFiles("shaders/simple.vert", "shaders/simple.frag")) {
        std::cerr << "Failed to load shader" << std::endl;
        return false;
    }
    createCube();
    return true;
}

void Renderer::createCube() {
    float vertices[] = {
        // positions          // normals       // texcoords
        -1.0f, -1.0f, -1.0f,  0,0,-1,  0.0f, 0.0f,
         1.0f, -1.0f, -1.0f,  0,0,-1,  1.0f, 0.0f,
         1.0f,  1.0f, -1.0f,  0,0,-1,  1.0f, 1.0f,
         1.0f,  1.0f, -1.0f,  0,0,-1,  1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f,  0,0,-1,  0.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,  0,0,-1,  0.0f, 0.0f,
        // ... (other faces omitted for brevity, but a real implementation should include all 36 verts)
    };
    // For skeleton purposes create a single triangle if full cube data is not present
    if (cubeVAO_) return;
    glGenVertexArrays(1, &cubeVAO_);
    glGenBuffers(1, &cubeVBO_);
    glBindVertexArray(cubeVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);
}

void Renderer::render(float time) {
    shader_.use();
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::vec3(0,0,5), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1280.0f/720.0f, 0.1f, 100.0f);
    GLuint pid = shader_.id();
    GLint loc;
    loc = glGetUniformLocation(pid, "uModel"); if (loc>=0) glUniformMatrix4fv(loc, 1, GL_FALSE, &model[0][0]);
    loc = glGetUniformLocation(pid, "uView"); if (loc>=0) glUniformMatrix4fv(loc, 1, GL_FALSE, &view[0][0]);
    loc = glGetUniformLocation(pid, "uProj"); if (loc>=0) glUniformMatrix4fv(loc, 1, GL_FALSE, &proj[0][0]);

    glBindVertexArray(cubeVAO_);
    // Draw the first triangle as placeholder
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}
