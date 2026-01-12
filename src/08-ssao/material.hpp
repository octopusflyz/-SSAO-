#pragma once

#include "../common/renderer.hpp"
#include "../common/shader.hpp"
#include "../common/texture.hpp"
#include "../common/framebuffer.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

class GeometryPassMaterial : public IMaterial {
public:
    glm::mat4 gWVP{};
    glm::mat4 gWV{};

    GeometryPassMaterial();
    void use() override;

private:
    std::unique_ptr<Program> _program;
    GLint _gWVPLocation;
    GLint _gWVLocation;
};

class SSAOMaterial : public IMaterial {
public:
    Texture2D* gPositionMap = nullptr;
    float gSampleRad = 0.5f;
    glm::mat4 gProj{};
    std::vector<glm::vec3> gKernel;

    SSAOMaterial();
    void use() override;
    void generateKernel();

private:
    std::unique_ptr<Program> _program;
    GLint _gPositionMapLocation;
    GLint _gSampleRadLocation;
    GLint _gProjLocation;
    GLint _gKernelLocation;
};

class BlurMaterial : public IMaterial {
public:
    Texture2D* gColorMap = nullptr;

    BlurMaterial();
    void use() override;

private:
    std::unique_ptr<Program> _program;
    GLint _gColorMapLocation;
};

class LightingMaterial : public IMaterial {
public:
    glm::mat4 gWVP{};
    glm::mat4 gWV{};
    glm::mat4 gWorld{};
    Texture2D* gAOMap = nullptr;
    glm::vec2 gScreenSize{};
    int gShaderType = 0; // 0: no SSAO, 1: SSAO, 2: show only AO

    struct BaseLight {
        glm::vec3 Color{1.0f, 1.0f, 1.0f};
        float AmbientIntensity = 0.1f;
        glm::vec3 Direction{0.5f, 0.5f, 0.5f};
        float DiffuseIntensity = 0.8f;
    } gLight;

    LightingMaterial();
    void use() override;

private:
    std::unique_ptr<Program> _program;
    GLint _gWVPLocation;
    GLint _gWVLocation;
    GLint _gWorldLocation;
    GLint _gAOMapLocation;
    GLint _gScreenSizeLocation;
    GLint _gShaderTypeLocation;
    GLint _gLightColorLocation;
    GLint _gLightAmbientIntensityLocation;
    GLint _gLightDirectionLocation;
    GLint _gLightDiffuseIntensityLocation;
};