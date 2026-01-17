#include "material.hpp"
#include "../common/gltf.hpp"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <sstream>

// Geometry Pass Material
GeometryPassMaterial::GeometryPassMaterial() {
  _program = Program::create_from_files("shaders/ssao_geometry.vert",
                                        "shaders/ssao_geometry.frag");
  _gWVPLocation = glGetUniformLocation(_program->get(), "gWVP");
  _gWVLocation = glGetUniformLocation(_program->get(), "gWV");

  // Material texture locations
  _gBaseColorLocation = glGetUniformLocation(_program->get(), "gBaseColor");
  _gNormalLocation = glGetUniformLocation(_program->get(), "gNormal");
  _gMetallicRoughnessLocation =
      glGetUniformLocation(_program->get(), "gMetallicRoughness");
  _gOcclusionLocation = glGetUniformLocation(_program->get(), "gOcclusion");
  _gEmissionLocation = glGetUniformLocation(_program->get(), "gEmission");

  // Material factor locations
  _gBaseColorFactorLocation =
      glGetUniformLocation(_program->get(), "gBaseColorFactor");
  _gMetallicFactorLocation =
      glGetUniformLocation(_program->get(), "gMetallicFactor");
  _gRoughnessFactorLocation =
      glGetUniformLocation(_program->get(), "gRoughnessFactor");
  _gNormalScaleLocation = glGetUniformLocation(_program->get(), "gNormalScale");
  _gOcclusionStrengthLocation =
      glGetUniformLocation(_program->get(), "gOcclusionStrength");
  _gEmissionFactorLocation =
      glGetUniformLocation(_program->get(), "gEmissionFactor");
}

void GeometryPassMaterial::use() {
  glUseProgram(_program->get());
  glUniformMatrix4fv(_gWVPLocation, 1, false, (GLfloat *)&gWVP);
  glUniformMatrix4fv(_gWVLocation, 1, false, (GLfloat *)&gWV);
}

void GeometryPassMaterial::setMaterialTextures(Gltf *scene,
                                               const Gltf::Material &mat) {
  // Helper: fetch texture safely with fallback to default white / flat normal
  auto getTextureIdSafe = [&](int idx, bool isNormal) {
    if (idx >= 0 && idx < (int)scene->textures.size()) {
      return scene->textures[idx]->get();
    }
    // Fallback: scene stores default textures at end: size-2 (white), size-1
    // (flat normal)
    int whiteIdx = (int)scene->textures.size() - 2;
    int normalIdx = (int)scene->textures.size() - 1;
    int fallbackIdx = isNormal ? normalIdx : whiteIdx;
    fallbackIdx =
        std::max(0, std::min(fallbackIdx, (int)scene->textures.size() - 1));
    return scene->textures[fallbackIdx]->get();
  };

  // Bind base color texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, getTextureIdSafe(mat.base_color, false));
  glUniform1i(_gBaseColorLocation, 0);

  // Bind normal texture
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, getTextureIdSafe(mat.normal, true));
  glUniform1i(_gNormalLocation, 1);

  // Bind metallic roughness texture
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, getTextureIdSafe(mat.metallic_roughness, false));
  glUniform1i(_gMetallicRoughnessLocation, 2);

  // Bind occlusion texture
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, getTextureIdSafe(mat.occlusion, false));
  glUniform1i(_gOcclusionLocation, 3);

  // Bind emission texture
  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, getTextureIdSafe(mat.emission, false));
  glUniform1i(_gEmissionLocation, 4);

  // Set material factors
  glUniform4f(_gBaseColorFactorLocation,
              mat.base_color_factor.r,
              mat.base_color_factor.g,
              mat.base_color_factor.b,
              mat.base_color_factor.a);
  glUniform1f(_gMetallicFactorLocation, mat.metallic_factor);
  glUniform1f(_gRoughnessFactorLocation, mat.roughness_factor);
  glUniform1f(_gNormalScaleLocation, mat.normal_scale);
  glUniform1f(_gOcclusionStrengthLocation, mat.occlusion_strength);
  glUniform3f(_gEmissionFactorLocation,
              mat.emission_factor.r,
              mat.emission_factor.g,
              mat.emission_factor.b);
}

// SSAO Material
SSAOMaterial::SSAOMaterial() {
  _program =
      Program::create_from_files("shaders/ssao.vert", "shaders/ssao.frag");
  _gPositionMapLocation = glGetUniformLocation(_program->get(), "gPositionMap");
  _gNormalMapLocation = glGetUniformLocation(_program->get(), "gNormalMap");
  _gSampleRadLocation = glGetUniformLocation(_program->get(), "gSampleRad");
  _gProjLocation = glGetUniformLocation(_program->get(), "gProj");
  _gKernelLocation = glGetUniformLocation(_program->get(), "gKernel[0]");

  generateKernel();
}

void SSAOMaterial::generateKernel() {
  std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
  std::default_random_engine generator;

  gKernel.resize(64);
  for (unsigned int i = 0; i < 64; ++i) {
    glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0,
                     randomFloats(generator) * 2.0 - 1.0,
                     randomFloats(generator));
    sample = glm::normalize(sample);
    sample *= randomFloats(generator);

    float scale = (float)i / 64.0;
    scale = glm::mix(0.1f, 1.0f, scale * scale);
    sample *= scale;

    gKernel[i] = sample;
  }
}

void SSAOMaterial::use() {
  glUseProgram(_program->get());

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,
                gPositionMap != nullptr ? gPositionMap->get() : 0);
  glUniform1i(_gPositionMapLocation, 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, gNormalMap != nullptr ? gNormalMap->get() : 0);
  glUniform1i(_gNormalMapLocation, 1);

  glUniform1f(_gSampleRadLocation, gSampleRad);
  glUniformMatrix4fv(_gProjLocation, 1, false, (GLfloat *)&gProj);

  // Set kernel uniforms
  for (size_t i = 0; i < gKernel.size(); ++i) {
    std::ostringstream oss;
    oss << "gKernel[" << i << "]";
    std::string uniformName = oss.str();
    GLint location = glGetUniformLocation(_program->get(), uniformName.c_str());
    glUniform3f(location, gKernel[i].x, gKernel[i].y, gKernel[i].z);
  }
}

// Blur Material
BlurMaterial::BlurMaterial() {
  _program = Program::create_from_files("shaders/ssao_blur.vert",
                                        "shaders/ssao_blur.frag");
  _gColorMapLocation = glGetUniformLocation(_program->get(), "gColorMap");
}

void BlurMaterial::use() {
  glUseProgram(_program->get());

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, gColorMap != nullptr ? gColorMap->get() : 0);
  glUniform1i(_gColorMapLocation, 0);
}

// SSDO Material
SSDOMaterial::SSDOMaterial() {
  _program =
      Program::create_from_files("shaders/ssao.vert", "shaders/ssdo.frag");
  _gPositionMapLocation = glGetUniformLocation(_program->get(), "gPositionMap");
  _gNormalMapLocation = glGetUniformLocation(_program->get(), "gNormalMap");
  _gAlbedoMapLocation = glGetUniformLocation(_program->get(), "gAlbedoMap");
  _gSampleRadLocation = glGetUniformLocation(_program->get(), "gSampleRad");
  _gProjLocation = glGetUniformLocation(_program->get(), "gProj");
  _gViewLocation = glGetUniformLocation(_program->get(), "gView");
  _gKernelLocation = glGetUniformLocation(_program->get(), "gKernel[0]");

  generateKernel();
}

void SSDOMaterial::generateKernel() {
  std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
  std::default_random_engine generator;

  gKernel.resize(64);
  for (unsigned int i = 0; i < 64; ++i) {
    glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0,
                     randomFloats(generator) * 2.0 - 1.0,
                     randomFloats(generator));
    sample = glm::normalize(sample);
    sample *= randomFloats(generator);

    float scale = (float)i / 64.0;
    scale = glm::mix(0.1f, 1.0f, scale * scale);
    sample *= scale;

    gKernel[i] = sample;
  }
}

void SSDOMaterial::use() {
  glUseProgram(_program->get());

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,
                gPositionMap != nullptr ? gPositionMap->get() : 0);
  glUniform1i(_gPositionMapLocation, 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, gNormalMap != nullptr ? gNormalMap->get() : 0);
  glUniform1i(_gNormalMapLocation, 1);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, gAlbedoMap != nullptr ? gAlbedoMap->get() : 0);
  glUniform1i(_gAlbedoMapLocation, 2);

  glUniform1f(_gSampleRadLocation, gSampleRad);
  glUniformMatrix4fv(_gProjLocation, 1, false, (GLfloat *)&gProj);
  glUniformMatrix4fv(_gViewLocation, 1, false, (GLfloat *)&gView);

  // Set kernel uniforms
  for (size_t i = 0; i < gKernel.size(); ++i) {
    std::ostringstream oss;
    oss << "gKernel[" << i << "]";
    std::string uniformName = oss.str();
    GLint location = glGetUniformLocation(_program->get(), uniformName.c_str());
    glUniform3f(location, gKernel[i].x, gKernel[i].y, gKernel[i].z);
  }
}

// Lighting Material
LightingMaterial::LightingMaterial() {
  _program = Program::create_from_files("shaders/ssao_lighting.vert",
                                        "shaders/ssao_lighting.frag");
  _gWVPLocation = glGetUniformLocation(_program->get(), "gWVP");
  _gWVLocation = glGetUniformLocation(_program->get(), "gWV");
  _gWorldLocation = glGetUniformLocation(_program->get(), "gWorld");
  _gAOMapLocation = glGetUniformLocation(_program->get(), "gAOMap");
  _gNormalMapLocation = glGetUniformLocation(_program->get(), "gNormalMap");
  _gAlbedoMapLocation = glGetUniformLocation(_program->get(), "gAlbedoMap");
  _gPositionMapLocation = glGetUniformLocation(_program->get(), "gPositionMap");
  _gScreenSizeLocation = glGetUniformLocation(_program->get(), "gScreenSize");
  _gShaderTypeLocation = glGetUniformLocation(_program->get(), "gShaderType");
  _gLightColorLocation = glGetUniformLocation(_program->get(), "gLight.Color");
  _gLightAmbientIntensityLocation =
      glGetUniformLocation(_program->get(), "gLight.AmbientIntensity");
  _gLightDirectionLocation =
      glGetUniformLocation(_program->get(), "gLight.Direction");
  _gLightDiffuseIntensityLocation =
      glGetUniformLocation(_program->get(), "gLight.DiffuseIntensity");
  _gNumPointLightsLocation =
      glGetUniformLocation(_program->get(), "gNumPointLights");
  _gPointLightPositionLocation =
      glGetUniformLocation(_program->get(), "gPointLights[0].Position");
  _gPointLightColorLocation =
      glGetUniformLocation(_program->get(), "gPointLights[0].Color");
  _gPointLightIntensityLocation =
      glGetUniformLocation(_program->get(), "gPointLights[0].Intensity");
  _gPointLightRadiusLocation =
      glGetUniformLocation(_program->get(), "gPointLights[0].Radius");

  // Material texture locations
  _gBaseColorLocation = glGetUniformLocation(_program->get(), "gBaseColor");
  _gNormalLocation = glGetUniformLocation(_program->get(), "gNormal");
  _gMetallicRoughnessLocation =
      glGetUniformLocation(_program->get(), "gMetallicRoughness");
  _gOcclusionLocation = glGetUniformLocation(_program->get(), "gOcclusion");
  _gEmissionLocation = glGetUniformLocation(_program->get(), "gEmission");

  // Material factor locations
  _gBaseColorFactorLocation =
      glGetUniformLocation(_program->get(), "gBaseColorFactor");
  _gMetallicFactorLocation =
      glGetUniformLocation(_program->get(), "gMetallicFactor");
  _gRoughnessFactorLocation =
      glGetUniformLocation(_program->get(), "gRoughnessFactor");
  _gNormalScaleLocation = glGetUniformLocation(_program->get(), "gNormalScale");
  _gOcclusionStrengthLocation =
      glGetUniformLocation(_program->get(), "gOcclusionStrength");
  _gEmissionFactorLocation =
      glGetUniformLocation(_program->get(), "gEmissionFactor");
}

void LightingMaterial::use() {
  glUseProgram(_program->get());

  glUniformMatrix4fv(_gWVPLocation, 1, false, (GLfloat *)&gWVP);
  glUniformMatrix4fv(_gWVLocation, 1, false, (GLfloat *)&gWV);
  glUniformMatrix4fv(_gWorldLocation, 1, false, (GLfloat *)&gWorld);

  // Bind G-Buffer textures
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, gAOMap != nullptr ? gAOMap->get() : 0);
  glUniform1i(_gAOMapLocation, 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, gNormalMap != nullptr ? gNormalMap->get() : 0);
  glUniform1i(_gNormalMapLocation, 1);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, gAlbedoMap != nullptr ? gAlbedoMap->get() : 0);
  glUniform1i(_gAlbedoMapLocation, 2);

  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D,
                gPositionMap != nullptr ? gPositionMap->get() : 0);
  glUniform1i(_gPositionMapLocation, 3);

  glUniform2f(_gScreenSizeLocation, gScreenSize.x, gScreenSize.y);
  glUniform1i(_gShaderTypeLocation, gShaderType);

  glUniform3f(
      _gLightColorLocation, gLight.Color.x, gLight.Color.y, gLight.Color.z);
  glUniform1f(_gLightAmbientIntensityLocation, gLight.AmbientIntensity);
  glUniform3f(_gLightDirectionLocation,
              gLight.Direction.x,
              gLight.Direction.y,
              gLight.Direction.z);
  glUniform1f(_gLightDiffuseIntensityLocation, gLight.DiffuseIntensity);

  // Set point lights
  glUniform1i(_gNumPointLightsLocation, gNumPointLights);
  for (int i = 0; i < gNumPointLights && i < 4; i++) {
    std::ostringstream oss;
    oss << "gPointLights[" << i << "]";
    std::string baseName = oss.str();

    glUniform3f(
        glGetUniformLocation(_program->get(), (baseName + ".Position").c_str()),
        gPointLights[i].Position.x,
        gPointLights[i].Position.y,
        gPointLights[i].Position.z);
    glUniform3f(
        glGetUniformLocation(_program->get(), (baseName + ".Color").c_str()),
        gPointLights[i].Color.x,
        gPointLights[i].Color.y,
        gPointLights[i].Color.z);
    glUniform1f(glGetUniformLocation(_program->get(),
                                     (baseName + ".Intensity").c_str()),
                gPointLights[i].Intensity);
    glUniform1f(
        glGetUniformLocation(_program->get(), (baseName + ".Radius").c_str()),
        gPointLights[i].Radius);
  }
}

void LightingMaterial::setMaterialTextures(Gltf *scene,
                                           const Gltf::Material &mat) {
  // Bind base color texture
  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, scene->textures[mat.base_color]->get());
  glUniform1i(_gBaseColorLocation, 4);

  // Bind normal texture
  glActiveTexture(GL_TEXTURE5);
  glBindTexture(GL_TEXTURE_2D, scene->textures[mat.normal]->get());
  glUniform1i(_gNormalLocation, 5);

  // Bind metallic roughness texture
  glActiveTexture(GL_TEXTURE6);
  glBindTexture(GL_TEXTURE_2D, scene->textures[mat.metallic_roughness]->get());
  glUniform1i(_gMetallicRoughnessLocation, 6);

  // Bind occlusion texture
  glActiveTexture(GL_TEXTURE7);
  glBindTexture(GL_TEXTURE_2D, scene->textures[mat.occlusion]->get());
  glUniform1i(_gOcclusionLocation, 7);

  // Bind emission texture
  glActiveTexture(GL_TEXTURE8);
  glBindTexture(GL_TEXTURE_2D, scene->textures[mat.emission]->get());
  glUniform1i(_gEmissionLocation, 8);

  // Set material factors
  glUniform4f(_gBaseColorFactorLocation,
              mat.base_color_factor.r,
              mat.base_color_factor.g,
              mat.base_color_factor.b,
              mat.base_color_factor.a);
  glUniform1f(_gMetallicFactorLocation, mat.metallic_factor);
  glUniform1f(_gRoughnessFactorLocation, mat.roughness_factor);
  glUniform1f(_gNormalScaleLocation, mat.normal_scale);
  glUniform1f(_gOcclusionStrengthLocation, mat.occlusion_strength);
  glUniform3f(_gEmissionFactorLocation,
              mat.emission_factor.r,
              mat.emission_factor.g,
              mat.emission_factor.b);
}