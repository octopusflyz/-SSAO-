#pragma once

#include "shader.h"
#include <string>
#include <glm/glm.hpp>

#include "gbuffer.h"

#include "model_loader.h"
#include "fbx_model.h"

struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float radius;
};

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    bool init();
    void render(float time, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);

    // Scene management
    bool initScene();
    void showDebugUI();
    void showDebugMode(int mode) { debugMode_ = mode; }

private:
    unsigned int cubeVAO_ = 0, cubeVBO_ = 0;
    unsigned int quadVAO_ = 0, quadVBO_ = 0;

    Shader shader_; // legacy/simple shader
    Shader gbufferShader_;
    Shader debugShader_;
    Shader lightingShader_; // for deferred lighting
    Shader ssaoShader_;     // for SSAO calculation
    Shader ssaoBlurShader_; // for SSAO blur

    GBuffer gbuffer_;
    unsigned int ssaoFBO_ = 0, ssaoBlurFBO_ = 0;
    unsigned int ssaoTex_ = 0, ssaoBlurTex_ = 0;
    unsigned int noiseTex_ = 0;

    int debugMode_ = 3; // 0=albedo,1=normal,2=depth,3=lighting,4=ssao

    // Scene data
    std::vector<glm::mat4> sceneTransforms_;
    std::vector<glm::vec3> sceneColors_;

    // Lighting system
    std::vector<PointLight> pointLights_;
    DirectionalLight directionalLight_;
    void setupLighting();
    void renderLighting(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
    void updateDynamicLighting(float time);

    // SSAO system
    void initSSAO();
    void renderSSAO(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
    void blurSSAO();

    void createCube();
    void createQuad();
    void renderQuad();
};
