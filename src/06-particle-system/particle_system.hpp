#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <random>

// 粒子结构体，包含粒子的基本属性
struct Particle {
    glm::vec3 position;    // 位置
    glm::vec3 velocity;    // 速度
    glm::vec3 acceleration; // 加速度
    float life;            // 生命周期 (0.0 - 1.0)
    float size;            // 大小
    glm::vec4 color;       // 颜色 (RGBA)
    float rotation;        // 旋转角度
    float rotationSpeed;   // 旋转速度
    
    // 构造函数
    Particle() : 
        position(0.0f), velocity(0.0f), acceleration(0.0f),
        life(1.0f), size(1.0f), color(1.0f), 
        rotation(0.0f), rotationSpeed(0.0f) {}
};

// 粒子发射器参数
struct EmitterParams {
    glm::vec3 position;           // 发射器位置
    glm::vec3 direction;          // 发射方向
    float spreadAngle;            // 扩散角度
    float emissionRate;           // 发射速率 (粒子/秒)
    float particleLifeTime;       // 粒子生命周期 (秒)
    glm::vec3 initialVelocity;   // 初始速度
    glm::vec3 acceleration;       // 加速度
    glm::vec4 startColor;         // 初始颜色
    glm::vec4 endColor;           // 结束颜色
    float startSize;              // 初始大小
    float endSize;                // 结束大小
    bool rotationEnabled;         // 是否启用旋转
    float rotationSpeedRange;     // 旋转速度范围
    
    // 构造函数，设置默认值
    EmitterParams() :
        position(0.0f, 0.0f, 0.0f),
        direction(0.0f, 1.0f, 0.0f),
        spreadAngle(30.0f),
        emissionRate(1000.0f),
        particleLifeTime(3.0f),
        initialVelocity(0.0f, 2.0f, 0.0f),
        acceleration(0.0f, -9.8f, 0.0f),
        startColor(1.0f, 0.5f, 0.0f, 1.0f),  // 橙色
        endColor(1.0f, 0.0f, 0.0f, 0.0f),    // 红色透明
        startSize(0.1f),
        endSize(0.05f),
        rotationEnabled(true),
        rotationSpeedRange(5.0f) {}
};

// 粒子系统类
class ParticleSystem {
public:
    // 构造函数
    ParticleSystem(int maxParticles = 10000);
    
    // 析构函数
    ~ParticleSystem();
    
    // 初始化粒子系统
    void initialize();
    
    // 更新粒子系统
    void update(float deltaTime);
    
    // 发射粒子
    void emit(int count);
    
    // 设置发射器参数
    void setEmitterParams(const EmitterParams& params);
    
    // 获取发射器参数
    const EmitterParams& getEmitterParams() const;
    
    // 获取粒子数据
    const std::vector<Particle>& getParticles() const;
    
    // 获取活跃粒子数量
    int getActiveParticleCount() const;
    
    // 重置粒子系统
    void reset();
    
    // 暂停/继续粒子系统
    void setPaused(bool paused);
    bool isPaused() const;
    
    // 设置最大粒子数量
    void setMaxParticles(int maxParticles);
    
    // 获取OpenGL缓冲区ID
    unsigned int getVBO() const;
    unsigned int getVAO() const;
    unsigned int getInstanceVBO() const;

private:
    // 更新单个粒子
    void updateParticle(Particle& particle, float deltaTime);
    
    // 生成随机数
    float randomFloat(float min, float max);
    glm::vec3 randomVectorInCone(const glm::vec3& direction, float angle);
    
    // 初始化OpenGL资源
    void initOpenGLResources();
    
    // 释放OpenGL资源
    void releaseOpenGLResources();
    
    // 更新实例数据缓冲区
    void updateInstanceBuffer();

private:
    std::vector<Particle> m_particles;        // 粒子数组
    std::vector<int> m_activeParticles;       // 活跃粒子索引
    EmitterParams m_emitterParams;            // 发射器参数
    
    int m_maxParticles;                        // 最大粒子数量
    float m_accumulatedTime;                   // 累积时间
    bool m_paused;                             // 是否暂停
    
    // 随机数生成器
    std::mt19937 m_randomGenerator;
    std::uniform_real_distribution<float> m_uniformDistribution;
    
    // OpenGL资源
    unsigned int m_VAO;                        // 顶点数组对象
    unsigned int m_VBO;                        // 顶点缓冲对象
    unsigned int m_instanceVBO;               // 实例缓冲对象
    
    bool m_initialized;                        // 是否已初始化
};

// 粒子特效类型枚举
enum class ParticleEffectType {
    FIRE,           // 火焰
    SMOKE,          // 烟雾
    EXPLOSION,      // 爆炸
    FOUNTAIN,       // 喷泉
    SNOW            // 雪花
};


class ParticleEffectFactory {
public:
    // 创建不同类型的粒子特效参数
    static EmitterParams createFireEffect();
    static EmitterParams createSmokeEffect();
    static EmitterParams createExplosionEffect();
    static EmitterParams createFountainEffect();
    static EmitterParams createSnowEffect();
};