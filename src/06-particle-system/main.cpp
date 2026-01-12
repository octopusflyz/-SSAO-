#include "../common/application.hpp"
#include "../common/framebuffer.hpp"
#include "../common/profile.h"
#include "../common/renderer.hpp"
#include "../common/shader.hpp"
#include "../common/utils.hpp"
#include "../05-pbr/material.hpp"
#include "particle_system.hpp"
#include "particle_material.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <iostream>
#include <sstream>
#include <vector>

class ParticleSystemApp final : public Application {
public:
    ParticleSystemApp() : Application("Particle System", 1200, 800) {}

private:
    //初始化
    void init() override {
        std::cout << "Initializing ParticleSystemApp..." << std::endl;
        
    
        std::cout << "Creating camera..." << std::endl;
        _camera = std::make_unique<ModelViewerCamera>();
        
    
        std::cout << "Creating renderer..." << std::endl;
        _renderer = std::make_unique<Renderer>();
        
    
        std::cout << "Creating particle system..." << std::endl;
        _particleSystem = std::make_unique<ParticleSystem>(10000);
        std::cout << "Initializing particle system..." << std::endl;
        _particleSystem->initialize();
        
        
        std::cout << "Creating particle material..." << std::endl;
        _particleMaterial = std::make_unique<ParticleMaterial>();
        std::cout << "Particle material created successfully." << std::endl;
        
     
        std::cout << "Setting initial particle effect..." << std::endl;
        setParticleEffect(ParticleEffectType::FIRE);
        std::cout << "Particle effect set." << std::endl;
        
  
        std::cout << "Creating tone mapping material..." << std::endl;
        _toneMappingMaterial = std::make_unique<ToneMappingMaterial>();
        std::cout << "Tone mapping material created." << std::endl;
        

        std::cout << "Initializing framebuffer..." << std::endl;
        _screen_fb_width = 0;
        _screen_fb_height = 0;
        _color_attachment = nullptr;
        _depth_stencil_attachment = nullptr;
        _framebuffer = nullptr;
        std::cout << "Framebuffer initialized." << std::endl;
        
  
        std::cout << "Initializing light parameters..." << std::endl;
        _lightYaw = glm::radians(45.0f);
        _lightPitch = glm::radians(45.0f);
        _lightIntensity = 1.0f;
        _lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
        _ambientStrength = 0.1f;
        _lightType = 0; // 光照类型，默认方向光
        _lightPosition = glm::vec3(2.0f, 2.0f, 2.0f); 
        std::cout << "Light parameters initialized." << std::endl;
        
    
        std::cout << "Initializing control parameters..." << std::endl;
        _currentEffect = ParticleEffectType::FIRE;
        _particleCount = 10000;
        _emissionRate = 500.0f;
        _particleLifeTime = 2.0f;
        _enableLighting = true;
        _particleAlpha = 1.0f;
        _paused = false;
        std::cout << "Control parameters initialized." << std::endl;
        
        std::cout << "Initializing mouse control..." << std::endl;
        _mouseButtonPressed = false;
        _lastMouseX = 0.0;
        _lastMouseY = 0.0;
        std::cout << "Mouse control initialized." << std::endl;
        
        std::cout << "ParticleSystemApp initialization complete." << std::endl;
    }

    void draw_ui() {
        int id = 0;
        
        // 显示FPS
        {
            std::stringstream ss;
            float frame_time = average_frame_time();
            ss << "FPS: ";
            if (frame_time == 0.0f) {
                ss << "NAN";
            } else {
                ss << 1.0f / frame_time;
            }
            ss << "(" << frame_time * 1000.0f << "ms)";
            ImGui::Text("%s", ss.str().c_str());
        }
        
        // 显示活跃粒子数量
        ImGui::Text("Active Particles: %d / %d", 
                   _particleSystem->getActiveParticleCount(), 
                   _particleCount);
        
        // 截图按钮
        if (ImGui::Button("Screen Shot")) {
            request_screen_shot();
        }
        
        // 切换性能分析器
        if (ImGui::Button("Toggle Profiler")) {
            toggle_profiler_ui();
        }
        
        // 相机控制
        if (ImGui::CollapsingHeader("Camera")) {
            ImGui::PushID(id++);
            _camera->draw_ui();
            ImGui::PopID();
        }
        
        // 粒子效果控制
        if (ImGui::CollapsingHeader("Particle Effect")) {
            ImGui::PushID(id++);
            
            // 粒子效果类型选择
            const char* effectTypes[] = { "Fire", "Smoke", "Explosion", "Fountain", "Snow" };
            if (ImGui::Combo("Effect Type", (int*)&_currentEffect, effectTypes, 5)) {
                setParticleEffect(_currentEffect);
            }
            
            // 粒子数量
            if (ImGui::SliderInt("Particle Count", &_particleCount, 1000, 50000)) {
                _particleSystem->setMaxParticles(_particleCount);
            }
            
            // 发射速率
            if (ImGui::SliderFloat("Emission Rate", &_emissionRate, 10.0f, 5000.0f)) {
                auto params = _particleSystem->getEmitterParams();
                params.emissionRate = _emissionRate;
                _particleSystem->setEmitterParams(params);
            }
            
            // 粒子生命周期
            if (ImGui::SliderFloat("Life Time", &_particleLifeTime, 0.5f, 10.0f)) {
                auto params = _particleSystem->getEmitterParams();
                params.particleLifeTime = _particleLifeTime;
                _particleSystem->setEmitterParams(params);
            }
            
            // 暂停/继续按钮
            if (ImGui::Button(_paused ? "Resume" : "Pause")) {
                _paused = !_paused;
                _particleSystem->setPaused(_paused);
            }
            
            // 重置按钮
            if (ImGui::Button("Reset")) {
                _particleSystem->reset();
            }
            
            ImGui::PopID();
        }
        
        // 光照控制
        if (ImGui::CollapsingHeader("Lighting")) {
            ImGui::PushID(id++);
            
            // 启用/禁用光照
            if (ImGui::Checkbox("Enable Lighting", &_enableLighting)) {
                auto renderParams = _particleMaterial->getRenderParams();
                renderParams.enableLighting = _enableLighting;
                _particleMaterial->setRenderParams(renderParams);
            }
            
            // 光照类型
            const char* lightTypes[] = { "Directional", "Point" };
            if (ImGui::Combo("Light Type", &_lightType, lightTypes, 2)) {
                update_light_params();
            }
            
            // 光照方向（方向光）
            if (_lightType == 0) {
                if (ImGui::SliderAngle("Light Yaw", &_lightYaw, 0.0f, 360.0f) ||
                    ImGui::SliderAngle("Light Pitch", &_lightPitch, -90.0f, 90.0f)) {
                    update_light_params();
                }
            }
            // 光照位置（点光源）
            else {
                if (ImGui::SliderFloat3("Light Position", &_lightPosition[0], -5.0f, 5.0f)) {
                    update_light_params();
                }
            }
            
            // 光照颜色
            if (ImGui::ColorEdit3("Light Color", &_lightColor[0])) {
                update_light_params();
            }
            
            // 光照强度
            if (ImGui::SliderFloat("Light Intensity", &_lightIntensity, 0.0f, 5.0f)) {
                update_light_params();
            }
            
            // 环境光强度
            if (ImGui::SliderFloat("Ambient Strength", &_ambientStrength, 0.0f, 1.0f)) {
                update_light_params();
            }
            
            ImGui::PopID();
        }
        
        // 渲染控制
        if (ImGui::CollapsingHeader("Rendering")) {
            ImGui::PushID(id++);
            
            // 粒子透明度
            if (ImGui::SliderFloat("Particle Alpha", &_particleAlpha, 0.0f, 1.0f)) {
                auto renderParams = _particleMaterial->getRenderParams();
                renderParams.particleAlpha = _particleAlpha;
                _particleMaterial->setRenderParams(renderParams);
            }
            
            ImGui::PopID();
        }
    }

    void update_light_params() {
        ParticleMaterial::LightParams lightParams;
        
        if (_lightType == 0) {
            // 方向光
            lightParams.type = ParticleMaterial::DIRECTIONAL;
            lightParams.direction = polar_to_cartesian(_lightYaw, _lightPitch);
        } else {
            // 点光源
            lightParams.type = ParticleMaterial::POINT;
            lightParams.position = _lightPosition;
        }
        
        lightParams.color = _lightColor;
        lightParams.intensity = _lightIntensity;
        lightParams.ambientStrength = _ambientStrength;
        
        _particleMaterial->setLightParams(lightParams);
    }

    void setParticleEffect(ParticleEffectType effectType) {
        _currentEffect = effectType;
        EmitterParams params;
        
        switch (effectType) {
        case ParticleEffectType::FIRE:
            params = ParticleEffectFactory::createFireEffect();
            break;
        case ParticleEffectType::SMOKE:
            params = ParticleEffectFactory::createSmokeEffect();
            break;
        case ParticleEffectType::EXPLOSION:
            params = ParticleEffectFactory::createExplosionEffect();
            break;
        case ParticleEffectType::FOUNTAIN:
            params = ParticleEffectFactory::createFountainEffect();
            break;
        case ParticleEffectType::SNOW:
            params = ParticleEffectFactory::createSnowEffect();
            break;
        }
        
        _particleSystem->setEmitterParams(params);
        _particleSystem->reset();
        
        // 更新UI参数
        _emissionRate = params.emissionRate;
        _particleLifeTime = params.particleLifeTime;
        
        // 设置相应的纹理
        auto& textureManager = ParticleTextureManager::getInstance();
        switch (effectType) {
        case ParticleEffectType::FIRE:
            _particleMaterial->setParticleTexture(textureManager.getFireTexture());
            break;
        case ParticleEffectType::SMOKE:
            _particleMaterial->setParticleTexture(textureManager.getSmokeTexture());
            break;
        case ParticleEffectType::EXPLOSION:
            _particleMaterial->setParticleTexture(textureManager.getExplosionTexture());
            break;
        case ParticleEffectType::FOUNTAIN:
            _particleMaterial->setParticleTexture(textureManager.getDefaultTexture());
            break;
        case ParticleEffectType::SNOW:
            _particleMaterial->setParticleTexture(textureManager.getSnowTexture());
            break;
        }
    }

    void draw_scene() {
        // 设置清屏颜色
        glm::vec3 env_radiance = glm::vec3(0.1f, 0.1f, 0.2f);  // 深蓝色背景
        glClearColor(env_radiance.x, env_radiance.y, env_radiance.z, 1.0);
        glViewport(0, 0, _screen_fb_width, _screen_fb_height);
        
  
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
     
        float aspect = (float)_screen_fb_width / (float)_screen_fb_height;
        glm::mat4 view = _camera->view();
        glm::mat4 projection = _camera->projection(aspect);
        
      
        _particleMaterial->model = glm::mat4(1.0f); 
        _particleMaterial->view = view;
        _particleMaterial->projection = projection;
        
        auto renderParams = _particleMaterial->getRenderParams();
        renderParams.viewPosition = _camera->position();
        _particleMaterial->setRenderParams(renderParams);
        
       
        _particleMaterial->use();
        
    
        if (_particleSystem->getActiveParticleCount() > 0) {
            glBindVertexArray(_particleSystem->getVAO());
            glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 
                                  _particleSystem->getActiveParticleCount());
            glBindVertexArray(0);
        }
    }

    void draw() {
     
        if (!_framebuffer) {
            return;
        }
        
     
        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer->get());
        draw_scene();
        
     
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (_toneMappingMaterial) {
            _renderer->blit(_color_attachment.get(), _toneMappingMaterial.get());
        } else {
            
            _renderer->blit(_color_attachment.get(), nullptr);
        }
    }

    void update_frame_buffer() {
        glfwGetFramebufferSize(_window, &_screen_fb_width, &_screen_fb_height);
        
        if (_color_attachment != nullptr &&
            _color_attachment->width() == _screen_fb_width &&
            _color_attachment->height() == _screen_fb_height) {
            return;
        }
        
  
        _color_attachment = std::make_unique<Texture2D>(nullptr,
                                                    GL_FLOAT,
                                                    _screen_fb_width,
                                                    _screen_fb_height,
                                                    GL_RGBA16F,
                                                    GL_RGBA);
        _depth_stencil_attachment = std::make_unique<Texture2D>(nullptr,
                                                              GL_UNSIGNED_INT_24_8,
                                                              _screen_fb_width,
                                                              _screen_fb_height,
                                                              GL_DEPTH24_STENCIL8,
                                                              GL_DEPTH_STENCIL);
        
        Texture2D *color_attachments[] = {_color_attachment.get()};
        _framebuffer = std::make_unique<Framebuffer>(color_attachments,
                                                  std::size(color_attachments),
                                                  _depth_stencil_attachment.get());
    }

    void update() override {
        update_frame_buffer();
        
   
        float deltaTime = average_frame_time();
        _particleSystem->update(deltaTime);
        
       
        update_light_params();
        
        draw_ui();
        draw();
    }

    void key_callback(int key, int scancode, int action, int mods) override {
        if (action == GLFW_PRESS) {
            switch (key) {
            case GLFW_KEY_SPACE:
                _paused = !_paused;
                _particleSystem->setPaused(_paused);
                break;
            case GLFW_KEY_R:
                _particleSystem->reset();
                break;
            case GLFW_KEY_1:
                setParticleEffect(ParticleEffectType::FIRE);
                break;
            case GLFW_KEY_2:
                setParticleEffect(ParticleEffectType::SMOKE);
                break;
            case GLFW_KEY_3:
                setParticleEffect(ParticleEffectType::EXPLOSION);
                break;
            case GLFW_KEY_4:
                setParticleEffect(ParticleEffectType::FOUNTAIN);
                break;
            case GLFW_KEY_5:
                setParticleEffect(ParticleEffectType::SNOW);
                break;
            }
        }
    }

    void mouse_button_callback(int button, int action, int mods) override {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                _mouseButtonPressed = true;
                double xpos, ypos;
                glfwGetCursorPos(_window, &xpos, &ypos);
                _lastMouseX = xpos;
                _lastMouseY = ypos;
            } else if (action == GLFW_RELEASE) {
                _mouseButtonPressed = false;
            }
        }
    }

    void cursor_position_callback(double xpos, double ypos) override {
        if (_mouseButtonPressed) {
            double deltaX = xpos - _lastMouseX;
            double deltaY = ypos - _lastMouseY;
            
            // 更新光源方向（当使用方向光时）
            if (_lightType == 0) {
                _lightYaw += deltaX * 0.01f;
                _lightPitch += deltaY * 0.01f;
                _lightPitch = glm::clamp(_lightPitch, -glm::pi<float>() / 2.0f, glm::pi<float>() / 2.0f);
                update_light_params();
            }
            
            _lastMouseX = xpos;
            _lastMouseY = ypos;
        }
    }

private:
 
    std::unique_ptr<ModelViewerCamera> _camera;
    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<ParticleSystem> _particleSystem;
    std::unique_ptr<ParticleMaterial> _particleMaterial;
    std::unique_ptr<ToneMappingMaterial> _toneMappingMaterial;
    
  
    int _screen_fb_width, _screen_fb_height;
    std::unique_ptr<Texture2D> _color_attachment;
    std::unique_ptr<Texture2D> _depth_stencil_attachment;
    std::unique_ptr<Framebuffer> _framebuffer;
    

    float _lightYaw;
    float _lightPitch;
    float _lightIntensity;
    glm::vec3 _lightColor;
    float _ambientStrength;
    int _lightType;
    glm::vec3 _lightPosition;
    

    ParticleEffectType _currentEffect;
    int _particleCount;
    float _emissionRate;
    float _particleLifeTime;
    bool _enableLighting;
    float _particleAlpha;
    bool _paused;

    bool _mouseButtonPressed;
    double _lastMouseX;
    double _lastMouseY;
};

int main() {
    try {
        std::cout << "Creating ParticleSystemApp..." << std::endl;
        ParticleSystemApp app{};
        std::cout << "Running app..." << std::endl;
        app.run();
        std::cout << "App finished." << std::endl;
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}