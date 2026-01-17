#pragma once

#include "../common/framebuffer.hpp"
#include "../common/gltf.hpp"
#include "../common/renderer.hpp"
#include "../common/shader.hpp"
#include "../common/texture.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class GeometryPassMaterial : public IMaterial {
public:
  glm::mat4 gWVP{};
  glm::mat4 gWV{};

  GeometryPassMaterial();
  void use() override;
  void setMaterialTextures(Gltf *scene, const Gltf::Material &mat);
  Program *getProgram() {
    return _program.get();
  }

private:
  std::unique_ptr<Program> _program;
  GLint _gWVPLocation;
  GLint _gWVLocation;
  GLint _gBaseColorLocation;
  GLint _gNormalLocation;
  GLint _gMetallicRoughnessLocation;
  GLint _gOcclusionLocation;
  GLint _gEmissionLocation;
  GLint _gBaseColorFactorLocation;
  GLint _gMetallicFactorLocation;
  GLint _gRoughnessFactorLocation;
  GLint _gNormalScaleLocation;
  GLint _gOcclusionStrengthLocation;
  GLint _gEmissionFactorLocation;
};

class SSAOMaterial : public IMaterial {
public:
  Texture2D *gPositionMap = nullptr;
  Texture2D *gNormalMap = nullptr;
  float gSampleRad = 0.5f;
  glm::mat4 gProj{};
  std::vector<glm::vec3> gKernel;

  SSAOMaterial();
  void use() override;
  void generateKernel();

private:
  std::unique_ptr<Program> _program;
  GLint _gPositionMapLocation;
  GLint _gNormalMapLocation;
  GLint _gSampleRadLocation;
  GLint _gProjLocation;
  GLint _gKernelLocation;
};

class SSDOMaterial : public IMaterial {
public:
  Texture2D *gPositionMap = nullptr;
  Texture2D *gNormalMap = nullptr;
  Texture2D *gAlbedoMap = nullptr;
  float gSampleRad = 0.5f;
  glm::mat4 gProj{};
  glm::mat4 gView{};
  std::vector<glm::vec3> gKernel;

  SSDOMaterial();
  void use() override;
  void generateKernel();

private:
  std::unique_ptr<Program> _program;
  GLint _gPositionMapLocation;
  GLint _gNormalMapLocation;
  GLint _gAlbedoMapLocation;
  GLint _gSampleRadLocation;
  GLint _gProjLocation;
  GLint _gViewLocation;
  GLint _gKernelLocation;
};

class BlurMaterial : public IMaterial {
public:
  Texture2D *gColorMap = nullptr;

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
  Texture2D *gAOMap = nullptr;
  Texture2D *gNormalMap = nullptr;
  Texture2D *gAlbedoMap = nullptr;
  Texture2D *gPositionMap = nullptr;
  glm::vec2 gScreenSize{};
  int gShaderType = 0; // 0: no SSAO, 1: SSAO, 2: show only AO

  struct BaseLight {
    glm::vec3 Color{1.0f, 1.0f, 1.0f};
    float AmbientIntensity = 0.1f;
    glm::vec3 Direction{0.5f, 0.5f, 0.5f};
    float DiffuseIntensity = 0.8f;
  } gLight;

  struct PointLight {
    glm::vec3 Position{0.0f, 0.0f, 0.0f};
    glm::vec3 Color{1.0f, 1.0f, 1.0f};
    float Intensity = 1.0f;
    float Radius = 10.0f;
  };
  std::vector<PointLight> gPointLights;
  int gNumPointLights = 0;

  LightingMaterial();
  void use() override;
  void setMaterialTextures(Gltf *scene, const Gltf::Material &mat);
  Program *getProgram() {
    return _program.get();
  }

private:
  std::unique_ptr<Program> _program;
  GLint _gWVPLocation;
  GLint _gWVLocation;
  GLint _gWorldLocation;
  GLint _gAOMapLocation;
  GLint _gNormalMapLocation;
  GLint _gAlbedoMapLocation;
  GLint _gPositionMapLocation;
  GLint _gScreenSizeLocation;
  GLint _gShaderTypeLocation;
  GLint _gLightColorLocation;
  GLint _gLightAmbientIntensityLocation;
  GLint _gLightDirectionLocation;
  GLint _gLightDiffuseIntensityLocation;
  GLint _gNumPointLightsLocation;
  GLint _gPointLightPositionLocation;
  GLint _gPointLightColorLocation;
  GLint _gPointLightIntensityLocation;
  GLint _gPointLightRadiusLocation;

  // Material texture locations
  GLint _gBaseColorLocation;
  GLint _gNormalLocation;
  GLint _gMetallicRoughnessLocation;
  GLint _gOcclusionLocation;
  GLint _gEmissionLocation;
  GLint _gBaseColorFactorLocation;
  GLint _gMetallicFactorLocation;
  GLint _gRoughnessFactorLocation;
  GLint _gNormalScaleLocation;
  GLint _gOcclusionStrengthLocation;
  GLint _gEmissionFactorLocation;
};