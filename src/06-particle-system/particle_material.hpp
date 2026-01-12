#pragma once

#include "../common/renderer.hpp"
#include "../common/shader.hpp"
#include "../common/texture.hpp"
#include <glm/glm.hpp>

// 粒子材质类，继承自IMaterial
class ParticleMaterial : public IMaterial {
public:
    // 光照类型枚举
    enum LightType {
        DIRECTIONAL,  // 方向光
        POINT         // 点光源
    };

    // 光照参数
    struct LightParams {
        LightType type;
        glm::vec3 position;      // 光源位置（点光源使用）
        glm::vec3 direction;     // 光源方向（方向光使用）
        glm::vec3 color;         // 光源颜色
        float intensity;          // 光照强度
        float ambientStrength;    // 环境光强度
        
        // 构造函数
        LightParams() : 
            type(DIRECTIONAL),
            position(1.0f, 1.0f, 1.0f),
            direction(-1.0f, -1.0f, -1.0f),
            color(1.0f, 1.0f, 1.0f),
            intensity(1.0f),
            ambientStrength(0.1f) {}
    };

    // 粒子渲染参数
    struct RenderParams {
        bool enableLighting;      // 是否启用光照
        bool enableBlending;      // 是否启用混合
        bool enableDepthTest;     // 是否启用深度测试
        bool enableDepthWrite;    // 是否启用深度写入
        glm::vec3 viewPosition;   // 观察者位置
        float particleAlpha;      // 粒子透明度系数
        
        // 构造函数
        RenderParams() :
            enableLighting(true),
            enableBlending(true),
            enableDepthTest(true),
            enableDepthWrite(false),
            viewPosition(0.0f, 0.0f, 3.0f),
            particleAlpha(1.0f) {}
    };

    // 构造函数
    ParticleMaterial();
    
    // 析构函数
    ~ParticleMaterial() = default;
    
    // 设置光照参数
    void setLightParams(const LightParams& params);
    
    // 获取光照参数
    const LightParams& getLightParams() const;
    
    // 设置渲染参数
    void setRenderParams(const RenderParams& params);
    
    // 获取渲染参数
    const RenderParams& getRenderParams() const;
    
    // 设置粒子纹理
    void setParticleTexture(Texture2D* texture);
    
    // 获取粒子纹理
    Texture2D* getParticleTexture() const;
    
    // 使用材质（设置着色器uniform变量）
    void use() override;

private:
    // 初始化着色器
    void initShader();
    
    // 获取uniform位置
    void getUniformLocations();

private:
    std::unique_ptr<Program> m_program;  // 着色器程序
    
    // Uniform位置
    GLint m_modelLocation;
    GLint m_viewLocation;
    GLint m_projectionLocation;
    GLint m_viewPositionLocation;
    
    // 光照相关uniform位置
    GLint m_lightTypeLocation;
    GLint m_lightPositionLocation;
    GLint m_lightDirectionLocation;
    GLint m_lightColorLocation;
    GLint m_lightIntensityLocation;
    GLint m_ambientStrengthLocation;
    
    // 渲染参数uniform位置
    GLint m_enableLightingLocation;
    GLint m_particleAlphaLocation;
    
    // 纹理uniform位置
    GLint m_particleTextureLocation;
    
    // 参数
    LightParams m_lightParams;
    RenderParams m_renderParams;
    Texture2D* m_particleTexture;
    
    bool m_initialized;  // 是否已初始化
};

// 粒子纹理管理器
class ParticleTextureManager {
public:
    // 获取单例实例
    static ParticleTextureManager& getInstance();
    
    // 获取默认粒子纹理
    Texture2D* getDefaultTexture();
    
    // 获取火焰纹理
    Texture2D* getFireTexture();
    
    // 获取烟雾纹理
    Texture2D* getSmokeTexture();
    
    // 获取爆炸纹理
    Texture2D* getExplosionTexture();
    
    // 获取雪花纹理
    Texture2D* getSnowTexture();

private:
    // 构造函数（私有，单例模式）
    ParticleTextureManager();
    
    // 析构函数
    ~ParticleTextureManager() = default;
    
    // 禁用拷贝构造和赋值
    ParticleTextureManager(const ParticleTextureManager&) = delete;
    ParticleTextureManager& operator=(const ParticleTextureManager&) = delete;
    
    // 创建程序化纹理
    void createDefaultTexture();
    void createFireTexture();
    void createSmokeTexture();
    void createExplosionTexture();
    void createSnowTexture();

private:
    std::unique_ptr<Texture2D> m_defaultTexture;
    std::unique_ptr<Texture2D> m_fireTexture;
    std::unique_ptr<Texture2D> m_smokeTexture;
    std::unique_ptr<Texture2D> m_explosionTexture;
    std::unique_ptr<Texture2D> m_snowTexture;
    
    bool m_initialized;
};