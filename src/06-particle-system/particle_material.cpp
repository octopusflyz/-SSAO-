#include "particle_material.hpp"
#include <GL/glew.h>
#include <iostream>

// ParticleMaterial实现
ParticleMaterial::ParticleMaterial() 
    : m_particleTexture(nullptr)
    , m_initialized(false) {
    initShader();
}

void ParticleMaterial::setLightParams(const LightParams& params) {
    m_lightParams = params;
}

const ParticleMaterial::LightParams& ParticleMaterial::getLightParams() const {
    return m_lightParams;
}

void ParticleMaterial::setRenderParams(const RenderParams& params) {
    m_renderParams = params;
}

const ParticleMaterial::RenderParams& ParticleMaterial::getRenderParams() const {
    return m_renderParams;
}

void ParticleMaterial::setParticleTexture(Texture2D* texture) {
    m_particleTexture = texture;
}

Texture2D* ParticleMaterial::getParticleTexture() const {
    return m_particleTexture;
}

void ParticleMaterial::use() {
    if (!m_initialized || !m_program) {
        return;
    }
    
    
    glUseProgram(m_program->get());
    

    glUniformMatrix4fv(m_modelLocation, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(m_viewLocation, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(m_projectionLocation, 1, GL_FALSE, &projection[0][0]);
    
   
    glUniform3fv(m_viewPositionLocation, 1, &m_renderParams.viewPosition[0]);
    
  
    glUniform1i(m_lightTypeLocation, static_cast<int>(m_lightParams.type));
    glUniform3fv(m_lightPositionLocation, 1, &m_lightParams.position[0]);
    glUniform3fv(m_lightDirectionLocation, 1, &m_lightParams.direction[0]);
    glUniform3fv(m_lightColorLocation, 1, &m_lightParams.color[0]);
    glUniform1f(m_lightIntensityLocation, m_lightParams.intensity);
    glUniform1f(m_ambientStrengthLocation, m_lightParams.ambientStrength);
    
    // 设置渲染参数
    glUniform1i(m_enableLightingLocation, m_renderParams.enableLighting ? 1 : 0);
    glUniform1f(m_particleAlphaLocation, m_renderParams.particleAlpha);
    
    // 设置纹理
    if (m_particleTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_particleTexture->get());
        glUniform1i(m_particleTextureLocation, 0);
    }
    

    if (m_renderParams.enableBlending) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
    
    if (m_renderParams.enableDepthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    
    glDepthMask(m_renderParams.enableDepthWrite ? GL_TRUE : GL_FALSE);
}

void ParticleMaterial::initShader() {
    if (m_initialized) {
        return;
    }
    
    // 顶点着色器代码
    const char* vertexShaderSource = R"(
#version 330 core

// Input vertex attributes
layout(location = 0) in vec3 aPosition;     // Vertex position
layout(location = 1) in vec2 aTexCoord;     // Texture coordinates

// Instanced attributes
layout(location = 2) in vec3 iPosition;     // Particle position
layout(location = 3) in float iSize;        // Particle size
layout(location = 4) in vec4 iColor;        // Particle color
layout(location = 5) in float iRotation;    // Particle rotation
layout(location = 6) in float iLife;        // Particle life

// Uniform variables
uniform mat4 model;              // Model matrix
uniform mat4 view;               // View matrix
uniform mat4 projection;         // Projection matrix
uniform vec3 viewPosition;       // Viewer position

// Output to fragment shader
out vec2 TexCoord;               // Texture coordinates
out vec4 ParticleColor;          // Particle color
out float ParticleLife;          // Particle life
out vec3 WorldPos;               // World position
out vec3 ViewPos;                // View space position
out vec3 Normal;                 // Normal (for lighting calculation)

void main() {
    // Calculate rotation matrix
    float cosR = cos(iRotation);
    float sinR = sin(iRotation);
    mat2 rotationMatrix = mat2(cosR, -sinR, sinR, cosR);
    
    // Apply rotation to vertex position
    vec2 rotatedPosition = rotationMatrix * aPosition.xy * iSize;
    
    // Calculate world position
    vec3 worldPos = iPosition + vec3(rotatedPosition, aPosition.z);
    
    // Calculate final position
    gl_Position = projection * view * model * vec4(worldPos, 1.0);
    
    // Pass texture coordinates
    TexCoord = aTexCoord;
    
    // Pass particle color and life
    ParticleColor = iColor;
    ParticleLife = iLife;
    
    // Calculate world and view space positions
    WorldPos = worldPos;
    ViewPos = vec3(view * model * vec4(worldPos, 1.0));
    
    // Calculate normal (facing viewer)
    Normal = normalize(viewPosition - worldPos);
}
)";

    // 片段着色器代码
    const char* fragmentShaderSource = R"(
#version 330 core

// Variables passed from vertex shader
in vec2 TexCoord;               // Texture coordinates
in vec4 ParticleColor;          // Particle color
in float ParticleLife;          // Particle life
in vec3 WorldPos;               // World position
in vec3 ViewPos;                // View space position
in vec3 Normal;                 // Normal

// Output color
out vec4 FragColor;

// Uniform variables
uniform sampler2D particleTexture;   // Particle texture
uniform bool enableLighting;          // Whether to enable lighting
uniform float particleAlpha;          // Particle alpha coefficient

// Lighting parameters
uniform int lightType;                // Light type (0=directional, 1=point)
uniform vec3 lightPosition;           // Light position
uniform vec3 lightDirection;          // Light direction
uniform vec3 lightColor;              // Light color
uniform float lightIntensity;         // Light intensity
uniform float ambientStrength;        // Ambient light strength

// Calculate lighting
vec3 calculateLighting(vec3 normal, vec3 viewDir, vec3 particleColor) {
    if (!enableLighting) {
        return particleColor;
    }
    
    vec3 lightDir;
    float attenuation = 1.0;
    
    if (lightType == 0) {
        // Directional light
        lightDir = normalize(-lightDirection);
    } else {
        // Point light
        vec3 lightVec = lightPosition - WorldPos;
        lightDir = normalize(lightVec);
        
        // Distance attenuation
        float distance = length(lightVec);
        attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    }
    
    // Ambient light
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse reflection
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular reflection (simple Phong model)
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spec * lightColor;
    
    // Combine lighting results
    vec3 result = (ambient + diffuse + specular) * particleColor * lightIntensity * attenuation;
    
    return result;
}

void main() {
    // Sample texture
    vec4 texColor = texture(particleTexture, TexCoord);
    
    // Discard if texture is transparent
    if (texColor.a < 0.01) {
        discard;
    }
    
    // Calculate final color
    vec3 finalColor;
    
    if (enableLighting) {
        // Calculate lighting
        vec3 viewDir = normalize(-ViewPos);
        finalColor = calculateLighting(Normal, viewDir, ParticleColor.rgb);
    } else {
        // Don't use lighting, directly use particle color
        finalColor = ParticleColor.rgb;
    }
    
    // Apply texture color
    finalColor *= texColor.rgb;
    
    // Calculate final alpha
    float finalAlpha = texColor.a * ParticleColor.a * ParticleLife * particleAlpha;
    
    // Output final color
    FragColor = vec4(finalColor, finalAlpha);
}
)";

    // 创建着色器
    Shader vertexShader(vertexShaderSource, GL_VERTEX_SHADER, "vertex");
    Shader fragmentShader(fragmentShaderSource, GL_FRAGMENT_SHADER, "fragment");
    
    // 创建着色器程序
    GLuint shaders[] = { vertexShader.get(), fragmentShader.get() };
    m_program = std::make_unique<Program>(shaders, 2);
    
    // 获取uniform位置
    getUniformLocations();
    
    m_initialized = true;
}

void ParticleMaterial::getUniformLocations() {

    m_modelLocation = glGetUniformLocation(m_program->get(), "model");
    m_viewLocation = glGetUniformLocation(m_program->get(), "view");
    m_projectionLocation = glGetUniformLocation(m_program->get(), "projection");
    m_viewPositionLocation = glGetUniformLocation(m_program->get(), "viewPosition");
    
  
    m_lightTypeLocation = glGetUniformLocation(m_program->get(), "lightType");
    m_lightPositionLocation = glGetUniformLocation(m_program->get(), "lightPosition");
    m_lightDirectionLocation = glGetUniformLocation(m_program->get(), "lightDirection");
    m_lightColorLocation = glGetUniformLocation(m_program->get(), "lightColor");
    m_lightIntensityLocation = glGetUniformLocation(m_program->get(), "lightIntensity");
    m_ambientStrengthLocation = glGetUniformLocation(m_program->get(), "ambientStrength");
    

    m_enableLightingLocation = glGetUniformLocation(m_program->get(), "enableLighting");
    m_particleAlphaLocation = glGetUniformLocation(m_program->get(), "particleAlpha");
    

    m_particleTextureLocation = glGetUniformLocation(m_program->get(), "particleTexture");
}


ParticleTextureManager& ParticleTextureManager::getInstance() {
    static ParticleTextureManager instance;
    return instance;
}

Texture2D* ParticleTextureManager::getDefaultTexture() {
    if (!m_initialized) {
        createDefaultTexture();
        m_initialized = true;
    }
    return m_defaultTexture.get();
}

Texture2D* ParticleTextureManager::getFireTexture() {
    if (!m_fireTexture) {
        createFireTexture();
    }
    return m_fireTexture.get();
}

Texture2D* ParticleTextureManager::getSmokeTexture() {
    if (!m_smokeTexture) {
        createSmokeTexture();
    }
    return m_smokeTexture.get();
}

Texture2D* ParticleTextureManager::getExplosionTexture() {
    if (!m_explosionTexture) {
        createExplosionTexture();
    }
    return m_explosionTexture.get();
}

Texture2D* ParticleTextureManager::getSnowTexture() {
    if (!m_snowTexture) {
        createSnowTexture();
    }
    return m_snowTexture.get();
}

ParticleTextureManager::ParticleTextureManager() : m_initialized(false) {
}

void ParticleTextureManager::createDefaultTexture() {
    const int size = 64;
    std::vector<unsigned char> data(size * size * 4);
    
    // 创建一个简单的圆形渐变纹理
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - size / 2.0f;
            float dy = y - size / 2.0f;
            float distance = sqrt(dx * dx + dy * dy);
            float maxDistance = size / 2.0f;
            
            int index = (y * size + x) * 4;
            
            if (distance < maxDistance) {
                float alpha = 1.0f - (distance / maxDistance);
                data[index + 0] = 255;     // R
                data[index + 1] = 255;     // G
                data[index + 2] = 255;     // B
                data[index + 3] = (unsigned char)(alpha * 255); // A
            } else {
                data[index + 0] = 0;
                data[index + 1] = 0;
                data[index + 2] = 0;
                data[index + 3] = 0;
            }
        }
    }
    
    m_defaultTexture = std::make_unique<Texture2D>(
        data.data(), GL_UNSIGNED_BYTE, size, size, GL_RGBA, GL_RGBA);
}

void ParticleTextureManager::createFireTexture() {
    const int size = 64;
    std::vector<unsigned char> data(size * size * 4);
    
    // 创建火焰纹理（从底部到顶部的渐变）
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - size / 2.0f;
            float dy = y - size / 2.0f;
            float distance = sqrt(dx * dx + dy * dy);
            float maxDistance = size / 2.0f;
            
            int index = (y * size + x) * 4;
            
            if (distance < maxDistance) {
                float alpha = 1.0f - (distance / maxDistance);
                float heightFactor = 1.0f - (y / (float)size);  // 从底部到顶部
                
                // 火焰颜色：底部黄色，中间橙色，顶部红色
                unsigned char r, g, b;
                if (heightFactor > 0.7f) {
                    // 底部：黄色
                    r = 255;
                    g = 255;
                    b = 0;
                } else if (heightFactor > 0.3f) {
                    // 中间：橙色
                    r = 255;
                    g = 128;
                    b = 0;
                } else {
                    // 顶部：红色
                    r = 255;
                    g = 0;
                    b = 0;
                }
                
                data[index + 0] = r;
                data[index + 1] = g;
                data[index + 2] = b;
                data[index + 3] = (unsigned char)(alpha * 255);
            } else {
                data[index + 0] = 0;
                data[index + 1] = 0;
                data[index + 2] = 0;
                data[index + 3] = 0;
            }
        }
    }
    
    m_fireTexture = std::make_unique<Texture2D>(
        data.data(), GL_UNSIGNED_BYTE, size, size, GL_RGBA, GL_RGBA);
}

void ParticleTextureManager::createSmokeTexture() {
    const int size = 64;
    std::vector<unsigned char> data(size * size * 4);
    
    // 创建烟雾纹理（灰色的噪声纹理）
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - size / 2.0f;
            float dy = y - size / 2.0f;
            float distance = sqrt(dx * dx + dy * dy);
            float maxDistance = size / 2.0f;
            
            int index = (y * size + x) * 4;
            
            if (distance < maxDistance) {
                // 使用简单的噪声函数
                float noise = sin(x * 0.1f) * cos(y * 0.1f) * 0.5f + 0.5f;
                float alpha = (1.0f - (distance / maxDistance)) * noise * 0.5f;
                
                // 烟雾颜色：灰色
                unsigned char gray = (unsigned char)(128 + noise * 64);
                
                data[index + 0] = gray;
                data[index + 1] = gray;
                data[index + 2] = gray;
                data[index + 3] = (unsigned char)(alpha * 255);
            } else {
                data[index + 0] = 0;
                data[index + 1] = 0;
                data[index + 2] = 0;
                data[index + 3] = 0;
            }
        }
    }
    
    m_smokeTexture = std::make_unique<Texture2D>(
        data.data(), GL_UNSIGNED_BYTE, size, size, GL_RGBA, GL_RGBA);
}

void ParticleTextureManager::createExplosionTexture() {
    const int size = 64;
    std::vector<unsigned char> data(size * size * 4);
    
    // 创建爆炸纹理（从中心向外的径向渐变）
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - size / 2.0f;
            float dy = y - size / 2.0f;
            float distance = sqrt(dx * dx + dy * dy);
            float maxDistance = size / 2.0f;
            
            int index = (y * size + x) * 4;
            
            if (distance < maxDistance) {
                float factor = distance / maxDistance;
                
                // 爆炸颜色：中心白色，中间黄色，边缘红色
                unsigned char r, g, b;
                if (factor < 0.3f) {
                    // 中心：白色
                    r = 255;
                    g = 255;
                    b = 255;
                } else if (factor < 0.7f) {
                    // 中间：黄色
                    r = 255;
                    g = 255;
                    b = 0;
                } else {
                    // 边缘：红色
                    r = 255;
                    g = 0;
                    b = 0;
                }
                
                float alpha = 1.0f - factor;
                
                data[index + 0] = r;
                data[index + 1] = g;
                data[index + 2] = b;
                data[index + 3] = (unsigned char)(alpha * 255);
            } else {
                data[index + 0] = 0;
                data[index + 1] = 0;
                data[index + 2] = 0;
                data[index + 3] = 0;
            }
        }
    }
    
    m_explosionTexture = std::make_unique<Texture2D>(
        data.data(), GL_UNSIGNED_BYTE, size, size, GL_RGBA, GL_RGBA);
}

void ParticleTextureManager::createSnowTexture() {
    const int size = 64;
    std::vector<unsigned char> data(size * size * 4);
    
    // 创建雪花纹理（六边形形状）
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - size / 2.0f;
            float dy = y - size / 2.0f;
            float distance = sqrt(dx * dx + dy * dy);
            float maxDistance = size / 2.0f;
            
            int index = (y * size + x) * 4;
            
            if (distance < maxDistance) {
                // 创建六边形形状
                float angle = atan2(dy, dx);
                float hexFactor = cos(angle * 3.0f) * 0.5f + 0.5f;
                float alpha = (1.0f - (distance / maxDistance)) * hexFactor;
                
                // 雪花颜色：白色
                data[index + 0] = 255;
                data[index + 1] = 255;
                data[index + 2] = 255;
                data[index + 3] = (unsigned char)(alpha * 255);
            } else {
                data[index + 0] = 0;
                data[index + 1] = 0;
                data[index + 2] = 0;
                data[index + 3] = 0;
            }
        }
    }
    
    m_snowTexture = std::make_unique<Texture2D>(
        data.data(), GL_UNSIGNED_BYTE, size, size, GL_RGBA, GL_RGBA);
}