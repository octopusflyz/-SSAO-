#include "material.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <algorithm>
#include <sstream>

// Geometry Pass Material
GeometryPassMaterial::GeometryPassMaterial() {
    _program = Program::create_from_files("shaders/ssao_geometry.vert", "shaders/ssao_geometry.frag");
    _gWVPLocation = glGetUniformLocation(_program->get(), "gWVP");
    _gWVLocation = glGetUniformLocation(_program->get(), "gWV");
}

void GeometryPassMaterial::use() {
    glUseProgram(_program->get());
    glUniformMatrix4fv(_gWVPLocation, 1, false, (GLfloat *)&gWVP);
    glUniformMatrix4fv(_gWVLocation, 1, false, (GLfloat *)&gWV);
}

// SSAO Material
SSAOMaterial::SSAOMaterial() {
    _program = Program::create_from_files("shaders/ssao.vert", "shaders/ssao.frag");
    _gPositionMapLocation = glGetUniformLocation(_program->get(), "gPositionMap");
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
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator)
        );
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
    glBindTexture(GL_TEXTURE_2D, gPositionMap != nullptr ? gPositionMap->get() : 0);
    glUniform1i(_gPositionMapLocation, 0);
    
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
    _program = Program::create_from_files("shaders/ssao_blur.vert", "shaders/ssao_blur.frag");
    _gColorMapLocation = glGetUniformLocation(_program->get(), "gColorMap");
}

void BlurMaterial::use() {
    glUseProgram(_program->get());
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gColorMap != nullptr ? gColorMap->get() : 0);
    glUniform1i(_gColorMapLocation, 0);
}

// Lighting Material
LightingMaterial::LightingMaterial() {
    _program = Program::create_from_files("shaders/ssao_lighting.vert", "shaders/ssao_lighting.frag");
    _gWVPLocation = glGetUniformLocation(_program->get(), "gWVP");
    _gWVLocation = glGetUniformLocation(_program->get(), "gWV");
    _gWorldLocation = glGetUniformLocation(_program->get(), "gWorld");
    _gAOMapLocation = glGetUniformLocation(_program->get(), "gAOMap");
    _gScreenSizeLocation = glGetUniformLocation(_program->get(), "gScreenSize");
    _gShaderTypeLocation = glGetUniformLocation(_program->get(), "gShaderType");
    _gLightColorLocation = glGetUniformLocation(_program->get(), "gLight.Color");
    _gLightAmbientIntensityLocation = glGetUniformLocation(_program->get(), "gLight.AmbientIntensity");
    _gLightDirectionLocation = glGetUniformLocation(_program->get(), "gLight.Direction");
    _gLightDiffuseIntensityLocation = glGetUniformLocation(_program->get(), "gLight.DiffuseIntensity");
}

void LightingMaterial::use() {
    glUseProgram(_program->get());
    
    glUniformMatrix4fv(_gWVPLocation, 1, false, (GLfloat *)&gWVP);
    glUniformMatrix4fv(_gWVLocation, 1, false, (GLfloat *)&gWV);
    glUniformMatrix4fv(_gWorldLocation, 1, false, (GLfloat *)&gWorld);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gAOMap != nullptr ? gAOMap->get() : 0);
    glUniform1i(_gAOMapLocation, 0);
    
    glUniform2f(_gScreenSizeLocation, gScreenSize.x, gScreenSize.y);
    glUniform1i(_gShaderTypeLocation, gShaderType);
    
    glUniform3f(_gLightColorLocation, gLight.Color.x, gLight.Color.y, gLight.Color.z);
    glUniform1f(_gLightAmbientIntensityLocation, gLight.AmbientIntensity);
    glUniform3f(_gLightDirectionLocation, gLight.Direction.x, gLight.Direction.y, gLight.Direction.z);
    glUniform1f(_gLightDiffuseIntensityLocation, gLight.DiffuseIntensity);
}