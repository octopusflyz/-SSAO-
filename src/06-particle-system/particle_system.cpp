#include "particle_system.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <glm/gtx/quaternion.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ParticleSystem::ParticleSystem(int maxParticles) 
    : m_maxParticles(maxParticles)
    , m_accumulatedTime(0.0f)
    , m_paused(false)
    , m_randomGenerator(42)  // 固定种子
    , m_uniformDistribution(0.0f, 1.0f)
    , m_VAO(0)
    , m_VBO(0)
    , m_instanceVBO(0)
    , m_initialized(false) {
    

    m_particles.resize(maxParticles);
    

    m_activeParticles.reserve(maxParticles);
}

ParticleSystem::~ParticleSystem() {
    releaseOpenGLResources();
}

void ParticleSystem::initialize() {
    if (m_initialized) {
        return;
    }
    
    initOpenGLResources();
    m_initialized = true;
}

void ParticleSystem::update(float deltaTime) {
    if (m_paused || !m_initialized) {
        return;
    }
    
    // 累积时间
    m_accumulatedTime += deltaTime;
    
    // 计算需要发射的粒子数量
    float emissionInterval = 1.0f / m_emitterParams.emissionRate;
    int particlesToEmit = static_cast<int>(m_accumulatedTime / emissionInterval);
    
    if (particlesToEmit > 0) {
        emit(particlesToEmit);
        m_accumulatedTime -= particlesToEmit * emissionInterval;
    }
    
    // 更新所有活跃粒子
    for (auto it = m_activeParticles.begin(); it != m_activeParticles.end(); ) {
        Particle& particle = m_particles[*it];
        updateParticle(particle, deltaTime);
        
        // 如果粒子生命周期结束，从活跃列表中移除
        if (particle.life <= 0.0f) {
            it = m_activeParticles.erase(it);
        } else {
            ++it;
        }
    }
    
    // 更新实例缓冲区
    updateInstanceBuffer();
}

void ParticleSystem::emit(int count) {
    int emitted = 0;
    
    // 找到未使用的粒子索引
    for (int i = 0; i < m_maxParticles && emitted < count; ++i) {
        // 检查粒子是否未被使用
        auto it = std::find(m_activeParticles.begin(), m_activeParticles.end(), i);
        if (it == m_activeParticles.end()) {
            // 初始化新粒子
            Particle& particle = m_particles[i];
            
            // 设置初始位置
            particle.position = m_emitterParams.position;
            
            // 设置初始速度（在指定方向的锥形范围内随机）
            glm::vec3 direction = randomVectorInCone(m_emitterParams.direction, m_emitterParams.spreadAngle);
            float speed = glm::length(m_emitterParams.initialVelocity);
            particle.velocity = direction * speed;
            
            // 设置加速度
            particle.acceleration = m_emitterParams.acceleration;
            
            // 设置生命周期
            particle.life = 1.0f;
            
            // 设置初始颜色和大小
            particle.color = m_emitterParams.startColor;
            particle.size = m_emitterParams.startSize;
            
            // 设置旋转
            if (m_emitterParams.rotationEnabled) {
                particle.rotation = randomFloat(0.0f, 2.0f * M_PI);
                particle.rotationSpeed = randomFloat(-m_emitterParams.rotationSpeedRange, m_emitterParams.rotationSpeedRange);
            } else {
                particle.rotation = 0.0f;
                particle.rotationSpeed = 0.0f;
            }
            
            // 添加到活跃粒子列表
            m_activeParticles.push_back(i);
            emitted++;
        }
    }
}

void ParticleSystem::setEmitterParams(const EmitterParams& params) {
    m_emitterParams = params;
}

const EmitterParams& ParticleSystem::getEmitterParams() const {
    return m_emitterParams;
}

const std::vector<Particle>& ParticleSystem::getParticles() const {
    return m_particles;
}

int ParticleSystem::getActiveParticleCount() const {
    return static_cast<int>(m_activeParticles.size());
}

void ParticleSystem::reset() {
    m_activeParticles.clear();
    m_accumulatedTime = 0.0f;
}

void ParticleSystem::setPaused(bool paused) {
    m_paused = paused;
}

bool ParticleSystem::isPaused() const {
    return m_paused;
}

void ParticleSystem::setMaxParticles(int maxParticles) {
    if (maxParticles != m_maxParticles) {
        m_maxParticles = maxParticles;
        m_particles.resize(maxParticles);
        m_activeParticles.clear();
        m_accumulatedTime = 0.0f;
        
      
        if (m_initialized) {
            releaseOpenGLResources();
            initOpenGLResources();
        }
    }
}

unsigned int ParticleSystem::getVBO() const {
    return m_VBO;
}

unsigned int ParticleSystem::getVAO() const {
    return m_VAO;
}

unsigned int ParticleSystem::getInstanceVBO() const {
    return m_instanceVBO;
}

void ParticleSystem::updateParticle(Particle& particle, float deltaTime) {
    // 更新生命周期
    float lifeDecrement = deltaTime / m_emitterParams.particleLifeTime;
    particle.life -= lifeDecrement;
    
    if (particle.life <= 0.0f) {
        return;
    }
    
    // 更新位置和速度
    particle.velocity += particle.acceleration * deltaTime;
    particle.position += particle.velocity * deltaTime;
    
    // 更新旋转
    particle.rotation += particle.rotationSpeed * deltaTime;
    
    // 更新颜色（线性插值）
    float t = 1.0f - particle.life;  // 从0到1
    particle.color = glm::mix(m_emitterParams.startColor, m_emitterParams.endColor, t);
    
    // 更新大小（线性插值）
    particle.size = glm::mix(m_emitterParams.startSize, m_emitterParams.endSize, t);
}

float ParticleSystem::randomFloat(float min, float max) {
    return min + m_uniformDistribution(m_randomGenerator) * (max - min);
}

glm::vec3 ParticleSystem::randomVectorInCone(const glm::vec3& direction, float angle) {
    // 将角度转换为弧度
    float angleRad = glm::radians(angle);
    
    // 生成随机角度
    float theta = randomFloat(0.0f, 2.0f * M_PI);
    float phi = randomFloat(0.0f, angleRad);
    
    // 计算球坐标
    float sinPhi = sin(phi);
    float x = sinPhi * cos(theta);
    float y = sinPhi * sin(theta);
    float z = cos(phi);
    
    // 创建局部坐标系中的向量
    glm::vec3 localDir(x, y, z);
    
    // 计算旋转到目标方向的四元数
    glm::vec3 normalizedDir = glm::normalize(direction);
    glm::vec3 defaultDir(0.0f, 0.0f, 1.0f);
    
    // 创建旋转四元数
    glm::quat rotation = glm::rotation(defaultDir, normalizedDir);
    
    // 旋转向量
    glm::vec3 rotatedDir = rotation * localDir;
    
    return glm::normalize(rotatedDir);
}

void ParticleSystem::initOpenGLResources() {
    // 创建顶点数据（一个简单的四边形）
    float vertices[] = {
        // 位置          // 纹理坐标
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    // 创建VAO
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);
    
    // 创建VBO
    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // 创建EBO
    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // 设置顶点属性指针
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 纹理坐标属性
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // 创建实例VBO
    glGenBuffers(1, &m_instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, m_maxParticles * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW);
    
    // 设置实例属性
    // 位置属性 (实例化)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, position));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    
    // 大小属性 (实例化)
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, size));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    
    // 颜色属性 (实例化)
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, color));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);
    
    // 旋转属性 (实例化)
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, rotation));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);
    
    // 生命周期属性 (实例化)
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, life));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);
    
    // 解绑
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // 删除EBO（不再需要）
    glDeleteBuffers(1, &EBO);
}

void ParticleSystem::releaseOpenGLResources() {
    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    
    if (m_VBO != 0) {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
    
    if (m_instanceVBO != 0) {
        glDeleteBuffers(1, &m_instanceVBO);
        m_instanceVBO = 0;
    }
}

void ParticleSystem::updateInstanceBuffer() {
    if (m_activeParticles.empty()) {
        return;
    }
    
    // 创建临时数组存储活跃粒子数据
    std::vector<Particle> activeParticles;
    activeParticles.reserve(m_activeParticles.size());
    
    for (int index : m_activeParticles) {
        activeParticles.push_back(m_particles[index]);
    }
    
    // 更新实例缓冲区
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, activeParticles.size() * sizeof(Particle), activeParticles.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// 粒子特效工厂实现
EmitterParams ParticleEffectFactory::createFireEffect() {
    EmitterParams params;
    params.position = glm::vec3(0.0f, 0.0f, 0.0f);
    params.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    params.spreadAngle = 15.0f;
    params.emissionRate = 500.0f;
    params.particleLifeTime = 2.0f;
    params.initialVelocity = glm::vec3(0.0f, 3.0f, 0.0f);
    params.acceleration = glm::vec3(0.0f, 0.5f, 0.0f);  // 向上的加速度
    params.startColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);  // 橙色
    params.endColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);    // 红色透明
    params.startSize = 0.2f;
    params.endSize = 0.05f;
    params.rotationEnabled = true;
    params.rotationSpeedRange = 3.0f;
    return params;
}

EmitterParams ParticleEffectFactory::createSmokeEffect() {
    EmitterParams params;
    params.position = glm::vec3(0.0f, 0.0f, 0.0f);
    params.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    params.spreadAngle = 25.0f;
    params.emissionRate = 200.0f;
    params.particleLifeTime = 4.0f;
    params.initialVelocity = glm::vec3(0.0f, 1.0f, 0.0f);
    params.acceleration = glm::vec3(0.0f, 0.2f, 0.0f);  // 缓慢上升
    params.startColor = glm::vec4(0.5f, 0.5f, 0.5f, 0.8f);  // 灰色半透明
    params.endColor = glm::vec4(0.8f, 0.8f, 0.8f, 0.0f);    // 浅灰色透明
    params.startSize = 0.3f;
    params.endSize = 0.8f;  // 烟雾扩散
    params.rotationEnabled = false;
    params.rotationSpeedRange = 0.0f;
    return params;
}

EmitterParams ParticleEffectFactory::createExplosionEffect() {
    EmitterParams params;
    params.position = glm::vec3(0.0f, 0.0f, 0.0f);
    params.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    params.spreadAngle = 180.0f;  // 全方向
    params.emissionRate = 2000.0f;  // 瞬间发射大量粒子
    params.particleLifeTime = 1.5f;
    params.initialVelocity = glm::vec3(0.0f, 5.0f, 0.0f);
    params.acceleration = glm::vec3(0.0f, -9.8f, 0.0f);  // 重力
    params.startColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);  // 黄色
    params.endColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);    // 红色透明
    params.startSize = 0.4f;
    params.endSize = 0.1f;
    params.rotationEnabled = true;
    params.rotationSpeedRange = 10.0f;
    return params;
}

EmitterParams ParticleEffectFactory::createFountainEffect() {
    EmitterParams params;
    params.position = glm::vec3(0.0f, 0.0f, 0.0f);
    params.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    params.spreadAngle = 20.0f;
    params.emissionRate = 800.0f;
    params.particleLifeTime = 3.0f;
    params.initialVelocity = glm::vec3(0.0f, 8.0f, 0.0f);
    params.acceleration = glm::vec3(0.0f, -9.8f, 0.0f);  // 重力
    params.startColor = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);  // 蓝色
    params.endColor = glm::vec4(0.0f, 0.8f, 1.0f, 0.0f);    // 浅蓝色透明
    params.startSize = 0.15f;
    params.endSize = 0.05f;
    params.rotationEnabled = false;
    params.rotationSpeedRange = 0.0f;
    return params;
}

EmitterParams ParticleEffectFactory::createSnowEffect() {
    EmitterParams params;
    params.position = glm::vec3(0.0f, 5.0f, 0.0f);  // 从上方开始
    params.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    params.spreadAngle = 10.0f;
    params.emissionRate = 300.0f;
    params.particleLifeTime = 10.0f;
    params.initialVelocity = glm::vec3(0.0f, -0.5f, 0.0f);
    params.acceleration = glm::vec3(0.0f, -0.1f, 0.0f);  // 缓慢下落
    params.startColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // 白色
    params.endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);    // 白色半透明
    params.startSize = 0.1f;
    params.endSize = 0.15f;
    params.rotationEnabled = true;
    params.rotationSpeedRange = 1.0f;
    return params;
}