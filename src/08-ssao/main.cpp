#include "../common/application.hpp"
#include "../common/framebuffer.hpp"
#include "../common/gltf.hpp"
#include "../common/renderer.hpp"
#include "../common/shader.hpp"
#include "material.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <iostream>
#include <tiny_gltf.h>
#include <vector>

struct FireballEffect {
  glm::vec3 position;
  glm::vec3 velocity;
  float lifetime; // seconds
  float maxLifetime = 5.0f; // 5 seconds lifetime
};

class SSAOApp final : public Application {
public:
  SSAOApp() : Application("SSAO Tutorial", 800, 600) {}

private:
  // Keyboard interaction
  void key_callback(int key, int scancode, int action, int mods) override {
    if (action == GLFW_PRESS || action == GLFW_RELEASE) {
      _keys[key] = (action == GLFW_PRESS);
    }

    // Toggle mouse-look with M key
    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
      _mouseLookEnabled = !_mouseLookEnabled;
      _firstMouse = true; // avoid sudden jump when re-enabled
    }

    // Toggle magic ring with 1 key (both main keyboard and numpad)
    if ((key == GLFW_KEY_1 || key == GLFW_KEY_KP_1) && action == GLFW_PRESS) {
      _showMagicRing = !_showMagicRing;
      printf("Magic ring toggled: %s\n", _showMagicRing ? "ON" : "OFF");
    }

    // Launch fireball with 2 key
    if ((key == GLFW_KEY_2 || key == GLFW_KEY_KP_2) && action == GLFW_PRESS) {
      // Calculate launch direction based on camera view
      glm::vec3 launchDirection;
      launchDirection.x = cos(_cameraPitch) * sin(_cameraYaw);
      launchDirection.y = sin(_cameraPitch);
      launchDirection.z = cos(_cameraPitch) * cos(_cameraYaw);
      launchDirection = glm::normalize(launchDirection);

      // Create fireball effect at player's eye position
      FireballEffect fireball;
      fireball.position = _playerPosition + glm::vec3(0.0f, 1.5f, 0.0f); // eye level
      fireball.velocity = launchDirection * 15.0f; // 15 units per second speed
      fireball.lifetime = 0.0f;

      _fireballEffects.push_back(fireball);
      printf("Fireball launched! Total effects: %zu\n", _fireballEffects.size());
    }
  }

  // Mouse interaction
  void cursor_position_callback(double xpos, double ypos) override {
    if (_firstMouse) {
      _lastMouseX = xpos;
      _lastMouseY = ypos;
      _firstMouse = false;
    }

    // If mouse-look is disabled, only update last position to avoid jump
    if (!_mouseLookEnabled) {
      _lastMouseX = xpos;
      _lastMouseY = ypos;
      return;
    }

    double deltaX = xpos - _lastMouseX;
    double deltaY = ypos - _lastMouseY;

    // Update camera based on mouse movement (first person)
    // Invert controls: mouse right = yaw left, mouse down = pitch up
    _cameraYaw -= deltaX * _mouseSensitivity;
    _cameraPitch -= deltaY * _mouseSensitivity;

    // Clamp pitch to avoid gimbal lock
    _cameraPitch = std::max(-glm::radians(89.0f),
                            std::min(_cameraPitch, glm::radians(89.0f)));

    _lastMouseX = xpos;
    _lastMouseY = ypos;
  }

  void mouse_button_callback(int button, int action, int mods) override {
    // First person camera doesn't need mouse button for rotation
  }

  void scroll_callback(double xoffset, double yoffset) override {
    // First person camera doesn't use scroll for zoom
  }

  void updatePlayerPosition(float deltaTime) {
    float moveSpeed = _moveSpeed * deltaTime;

    // Calculate front and right vectors
    glm::vec3 front;
    front.x = cos(_cameraPitch) * sin(_cameraYaw);
    front.y = 0.0f; // Don't move up/down
    front.z = cos(_cameraPitch) * cos(_cameraYaw);
    front = glm::normalize(front);

    glm::vec3 right =
        glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

    // Check if moving
    _isMoving = false;
    _isMovingVertical = false;
    glm::vec3 oldPosition = _playerPosition;

    // WASD movement (horizontal)
    if (_keys[GLFW_KEY_W]) {
      _playerPosition += front * moveSpeed;
      _isMoving = true;
    }
    if (_keys[GLFW_KEY_S]) {
      _playerPosition -= front * moveSpeed;
      _isMoving = true;
    }
    if (_keys[GLFW_KEY_A]) {
      _playerPosition -= right * moveSpeed;
      _isMoving = true;
    }
    if (_keys[GLFW_KEY_D]) {
      _playerPosition += right * moveSpeed;
      _isMoving = true;
    }

    // QE movement (vertical)
    if (_keys[GLFW_KEY_Q]) {
      _playerPosition.y -= moveSpeed;
      _isMovingVertical = true;
    }
    if (_keys[GLFW_KEY_E]) {
      _playerPosition.y += moveSpeed;
      _isMovingVertical = true;
    }

    // Update animation time if moving
    if (_isMoving || _isMovingVertical) {
      _animationTime += deltaTime * 5.0f; // Animation speed
    }
  }

  // Get character animation transform
  glm::mat4 getCharacterAnimationTransform() const {
    if (!_isMoving && !_isMovingVertical) {
      return glm::identity<glm::mat4>();
    }

    // Simple walking animation: bob up and down
    float bobAmount = 0.05f;
    float bobSpeed = _animationTime;

    // Vertical bobbing for walking
    float verticalOffset = sin(bobSpeed) * bobAmount;

    // Slight forward/backward tilt for walking
    float tiltAmount = 0.02f;
    float tiltOffset = sin(bobSpeed * 2.0f) * tiltAmount;

    // For vertical movement (QE), add different animation
    if (_isMovingVertical) {
      verticalOffset = sin(bobSpeed * 0.5f) * bobAmount * 1.5f;
    }

    // Create animation transform
    glm::mat4 animTransform = glm::identity<glm::mat4>();
    animTransform =
        glm::translate(animTransform, glm::vec3(0.0f, verticalOffset, 0.0f));
    animTransform =
        glm::rotate(animTransform, tiltOffset, glm::vec3(1.0f, 0.0f, 0.0f));

    return animTransform;
  }

  glm::mat4 getThirdPersonCameraView() const {
    // Calculate front direction from camera angles
    glm::vec3 front;
    front.x = cos(_cameraPitch) * sin(_cameraYaw);
    front.y = sin(_cameraPitch);
    front.z = cos(_cameraPitch) * cos(_cameraYaw);
    front = glm::normalize(front);

    glm::vec3 right =
        glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    // Third person camera: position behind and to the right of player
    float cameraDistance = 4.0f; // Distance behind player
    float cameraHeight = 2.0f;   // Height above player
    float cameraOffset = 1.5f;   // Offset to the right

    glm::vec3 cameraPos = _playerPosition +
                          glm::vec3(0.0f, cameraHeight, 0.0f) -
                          front * cameraDistance + right * cameraOffset;

    // Look at a point slightly above player position
    glm::vec3 lookAtPos = _playerPosition + glm::vec3(0.0f, 1.5f, 0.0f);

    return glm::lookAt(cameraPos, lookAtPos, up);
  }

  // Get character rotation based on camera direction
  float getCharacterRotation() const {
    // Character should face the same direction as camera is looking
    // Convert camera yaw to character rotation (in degrees)
    return glm::degrees(_cameraYaw);
  }
  void init() override {
    // Load multiple models
    _scenes.push_back(std::make_unique<Gltf>(
        "models/old_church_modeling_-_interior_scene/scene.gltf"));
    _scenes.push_back(std::make_unique<Gltf>(
        "models/elaina_-_the_witchs_journey/scene.gltf"));

    // 添加魔法阵模型
    _scenes.push_back(std::make_unique<Gltf>(
        "models/magic_ring_-_red/scene.gltf"));

    // 添加火球模型
    _scenes.push_back(std::make_unique<Gltf>(
        "models/fireball/scene.gltf"));

    // Print scene info for debugging
    std::cout << "Loaded " << _scenes.size() << " scenes" << std::endl;
    for (size_t i = 0; i < _scenes.size(); ++i) {
      std::cout << "Scene " << i << ": " << _scenes[i]->meshes.size()
                << " meshes, " << _scenes[i]->draws.size() << " draws, "
                << _scenes[i]->materials.size() << " materials" << std::endl;
    }
    _renderer = std::make_unique<Renderer>();

    // Initialize player position (inside the room)
    _playerPosition = glm::vec3(
        0.0f, 1.7f, 3.0f); // 1.7m eye height, positioned inside the room
    _cameraYaw = glm::radians(180.0f); // Looking towards the center
    _cameraPitch = glm::radians(0.0f); // Looking straight ahead

    // Initialize materials
    _geometryMaterial = std::make_unique<GeometryPassMaterial>();
    _ssaoMaterial = std::make_unique<SSAOMaterial>();
    _blurMaterial = std::make_unique<BlurMaterial>();
    _lightingMaterial = std::make_unique<LightingMaterial>();

    // Brighter default lighting for visibility (keep within UI slider range)
    _lightingMaterial->gLight.AmbientIntensity = 1.8f;
    _lightingMaterial->gLight.DiffuseIntensity = 1.3f;

    // Create full screen triangle
    std::vector<Mesh::Vertex> vertices = {
        {{-1.0f, -1.0f, 0.0f}, {}, {}, {0.0f, 0.0f}}, // left-down
        {{3.0f, -1.0f, 0.0f}, {}, {}, {2.0f, 0.0f}},  // right-down
        {{-1.0f, 3.0f, 0.0f}, {}, {}, {0.0f, 2.0f}},  // left-up
    };

    _fullScreenTriangle = std::make_unique<Mesh>(
        vertices.data(), (uint32_t)vertices.size(), nullptr, 0);

    // Initialize light direction
    _lightYaw = glm::radians(60.0f);
    _lightPitch = glm::radians(60.0f);
    updateLightDirection();

    // Initialize point lights
    // Only keep camera light - remove other lights
    LightingMaterial::PointLight cameraLight;
    cameraLight.Position =
        _playerPosition + glm::vec3(0.0f, _playerHeight, 0.0f);
    cameraLight.Color = glm::vec3(1.0f, 0.95f, 0.8f); // Warm white
    cameraLight.Intensity = 15.0f; // Stronger player-follow light
    cameraLight.Radius = 60.0f;     // Softer falloff to avoid dark rim
    _lightingMaterial->gPointLights.push_back(cameraLight);

    _lightingMaterial->gNumPointLights =
        (int)_lightingMaterial->gPointLights.size();
  }

  void draw_ui() {
    ImGui::Text("SSAO Tutorial - Third Person");
    ImGui::Separator();

    // 显示加载的模型数量
    ImGui::Text("Loaded Models: %d", (int)_scenes.size());
    if (_scenes.size() >= 4) {
      ImGui::Text("1. Church Interior Scene");
      ImGui::Text("2. Character (Elaina)");
      ImGui::Text("3. Magic Circle (Red) - Ground Effect");
      ImGui::Text("4. Fireball - Front Effect");
    }
    ImGui::Separator();

    // Camera Controls
    if (ImGui::CollapsingHeader("Controls")) {
      ImGui::Text("WASD: Move horizontal");
      ImGui::Text("QE: Move up/down");
      ImGui::Text("Mouse: Look around");
      ImGui::Text("ESC: Exit");
      ImGui::Text("Player Position: (%.2f, %.2f, %.2f)",
                  _playerPosition.x,
                  _playerPosition.y,
                  _playerPosition.z);
      ImGui::Text("Camera Yaw: %.1f°", glm::degrees(_cameraYaw));
      ImGui::Text("Camera Pitch: %.1f°", glm::degrees(_cameraPitch));

      ImGui::Separator();
      ImGui::SliderFloat("Move Speed", &_moveSpeed, 1.0f, 10.0f);
      ImGui::SliderFloat(
          "Mouse Sensitivity", &_mouseSensitivity, 0.001f, 0.01f);
    }

    // SSAO Settings
    if (ImGui::CollapsingHeader("SSAO Settings")) {
      ImGui::SliderFloat(
          "Sample Radius", &_ssaoMaterial->gSampleRad, 0.1f, 2.0f);
      ImGui::Text("Recommended: 0.3-0.5 for indoor scenes");
    }

    // Light Settings
    if (ImGui::CollapsingHeader("Light Settings")) {
      ImGui::ColorEdit3("Light Color",
                        (float *)&_lightingMaterial->gLight.Color);
      ImGui::SliderFloat("Ambient Intensity",
                         &_lightingMaterial->gLight.AmbientIntensity,
                         0.0f,
                         1.0f);
      ImGui::SliderFloat("Diffuse Intensity",
                         &_lightingMaterial->gLight.DiffuseIntensity,
                         0.0f,
                         2.0f);

      ImGui::Separator();
      ImGui::Text("Light Direction");
      if (ImGui::SliderAngle("Light Yaw", &_lightYaw, 0.0f, 360.0f)) {
        updateLightDirection();
      }
      if (ImGui::SliderAngle("Light Pitch", &_lightPitch, -89.0f, 89.0f)) {
        updateLightDirection();
      }
    }

    // Render Mode
    if (ImGui::CollapsingHeader("Render Mode")) {
      const char *items[] = {"No SSAO", "SSAO", "Show AO Only"};
      if (ImGui::Combo("Mode", &_shaderType, items, IM_ARRAYSIZE(items))) {
        _lightingMaterial->gShaderType = _shaderType;
      }
    }
  }

  void updateLightDirection() {
    glm::vec3 lightDir = glm::vec3(cos(_lightPitch) * sin(_lightYaw),
                                   sin(_lightPitch),
                                   cos(_lightPitch) * cos(_lightYaw));
    _lightingMaterial->gLight.Direction = glm::normalize(lightDir);
  }

  void GeometryPass() {
    for (size_t sceneIdx = 0; sceneIdx < _scenes.size(); ++sceneIdx) {
      auto &scene = _scenes[sceneIdx];
      for (auto &draw : scene->draws) {
        // Apply scene-specific transforms
        glm::mat4 sceneTransform = draw.transform;

        // For fantasy_interior_kit (scene), adjust position
        if (sceneIdx == 0) {
          // Scale the scene to 2.0x (double size)
          glm::mat4 sceneScale = glm::scale(glm::identity<glm::mat4>(),
                                            glm::vec3(2.0f, 2.0f, 2.0f));
          glm::mat4 sceneTranslate = glm::translate(
              glm::identity<glm::mat4>(), glm::vec3(0.0f, 0.0f, 0.0f));
          sceneTransform = sceneTranslate * sceneScale * draw.transform;
        }

        // For elaina (character), position relative to player
        if (sceneIdx == 1) { // elaina is the second model
          // Scale down the character
          glm::mat4 characterScale = glm::scale(glm::identity<glm::mat4>(),
                                                glm::vec3(0.3f, 0.3f, 0.3f));
          // Rotate to make character stand upright (rotate 90 degrees around X
          // axis to flip upside down)
          glm::mat4 characterRotate = glm::rotate(glm::identity<glm::mat4>(),
                                                  glm::radians(90.0f),
                                                  glm::vec3(1.0f, 0.0f, 0.0f));
          // Character rotates with camera (face the direction camera is
          // looking)
          float characterYaw = getCharacterRotation();
          glm::mat4 characterRotateY = glm::rotate(glm::identity<glm::mat4>(),
                                                   glm::radians(characterYaw),
                                                   glm::vec3(0.0f, 1.0f, 0.0f));
          // Keep animation in geometry pass so G-Buffer matches lighting pass
          glm::mat4 characterAnim = getCharacterAnimationTransform();
          // Position elaina at player position (character is the player)
          glm::mat4 characterTranslate =
              glm::translate(glm::identity<glm::mat4>(),
                           _playerPosition + glm::vec3(0.0f, 0.0f, 0.0f));
          sceneTransform = characterTranslate * characterRotateY *
                           characterRotate * characterAnim * characterScale *
                           draw.transform;
        }

        // 添加魔法阵渲染逻辑（按1键切换显示）
        if (sceneIdx == 2) { // magic_ring is the third model
          if (_showMagicRing) {
            // 显示魔法阵：应用特殊变换
            // 缩小魔法阵到合适大小
            glm::mat4 ringScale = glm::scale(glm::identity<glm::mat4>(),
                                            glm::vec3(0.5f, 0.5f, 0.5f));

            // 旋转魔法阵使其水平放置在地面上
            glm::mat4 ringRotate = glm::rotate(glm::identity<glm::mat4>(),
                                              glm::radians(90.0f),
                                              glm::vec3(0.0f, 1.0f, 0.0f));

            // 添加旋转动画效果（围绕Y轴缓慢旋转）
            static float ringRotationAngle = 0.0f;
            ringRotationAngle += 0.01f; // 每帧增加一点旋转
            if (ringRotationAngle > 360.0f) ringRotationAngle -= 360.0f;

            glm::mat4 ringAnimRotate = glm::rotate(glm::identity<glm::mat4>(),
                                                  ringRotationAngle,
                                                  glm::vec3(0.0f, 1.0f, 0.0f));

            // 添加上下浮动动画
            float floatOffset = sin(ringRotationAngle * 2.0f) * 0.05f; // 轻微浮动

            // 将魔法阵放置在角色脚下（稍微下沉到地面以下一点）
            glm::vec3 ringPosition = _playerPosition;
            ringPosition.y = _playerPosition.y - 0.8f + floatOffset; // 相对于玩家位置，稍微下沉 + 浮动

            glm::mat4 ringTranslate = glm::translate(glm::identity<glm::mat4>(),
                                                    ringPosition);

            // 组合所有变换：位置 -> 动画旋转 -> 水平旋转 -> 缩放 -> 原始变换
            sceneTransform = ringTranslate * ringAnimRotate * ringRotate *
                           ringScale * draw.transform;
          } else {
            // 不显示魔法阵：跳过渲染
            continue;
          }
        }

        // 添加火球渲染逻辑
        if (sceneIdx == 3) { // fireball is the fourth model
          // 如果没有活跃的火球效果，不渲染fireball模型
          if (_fireballEffects.empty()) {
            continue;
          }

          // 为每个活跃的火球效果渲染
          for (const auto& fireball : _fireballEffects) {
            // 缩小火球到合适大小
            glm::mat4 fireballScale = glm::scale(glm::identity<glm::mat4>(),
                                                glm::vec3(0.3f, 0.3f, 0.3f));

            // 添加旋转动画（基于生命周期）
            float fireballRotationAngle = fireball.lifetime * 2.0f; // 旋转速度与时间相关

            glm::mat4 fireballRotate = glm::rotate(glm::identity<glm::mat4>(),
                                                  fireballRotationAngle,
                                                  glm::vec3(0.0f, 1.0f, 0.0f));

            glm::mat4 fireballTranslate = glm::translate(glm::identity<glm::mat4>(),
                                                        fireball.position);

            // 组合变换：位置 -> 旋转 -> 缩放 -> 原始变换
            sceneTransform = fireballTranslate * fireballRotate * fireballScale * draw.transform;

            // 渲染这个火球
            _geometryMaterial->gWVP = _projection * _view * sceneTransform;
            _geometryMaterial->gWV = _view * sceneTransform;
            _geometryMaterial->use();

            // Set material textures and render state
            int materialIndex = scene->meshes[draw.index][0].material;
            if (materialIndex >= 0 && materialIndex < scene->materials.size()) {
              auto &mat = *scene->materials[materialIndex];

              // 保存当前OpenGL状态
              GLboolean blendEnabled;
              GLboolean depthTestEnabled;
              glGetBooleanv(GL_BLEND, &blendEnabled);
              glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);

              // Set render state based on material mode
              if (mat.mode == Gltf::Material::Blend) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable(GL_DEPTH_TEST);
              } else {
                glDisable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
              }

              _geometryMaterial->setMaterialTextures(scene.get(), mat);

              // Render the mesh
              for (auto &prim : scene->meshes[draw.index]) {
                prim.mesh->draw();
              }

              // 恢复OpenGL状态
              if (blendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
              if (depthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
            }
          }

          // 跳过fireball模型的正常渲染，因为我们已经在上面渲染了所有fireball实例
          continue;
        }

        _geometryMaterial->gWVP = _projection * _view * sceneTransform;
        _geometryMaterial->gWV = _view * sceneTransform;
        _geometryMaterial->use();

        // Set material textures and render state
        int materialIndex = scene->meshes[draw.index][0].material;
        if (materialIndex >= 0 && materialIndex < scene->materials.size()) {
          auto &mat = *scene->materials[materialIndex];

          // Set render state based on material mode
          if (mat.mode == Gltf::Material::Blend) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_DEPTH_TEST);
          } else {
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
          }

          _geometryMaterial->setMaterialTextures(scene.get(), mat);
        } else {
          // Use default white texture if no material
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D,
                        scene->textures[scene->textures.size() - 2]->get());
          glUniform1i(glGetUniformLocation(
                          _geometryMaterial->getProgram()->get(), "gBaseColor"),
                      0);
          glActiveTexture(GL_TEXTURE1);
          glBindTexture(GL_TEXTURE_2D,
                        scene->textures[scene->textures.size() - 1]->get());
          glUniform1i(glGetUniformLocation(
                          _geometryMaterial->getProgram()->get(), "gNormal"),
                      1);
          glActiveTexture(GL_TEXTURE2);
          glBindTexture(GL_TEXTURE_2D,
                        scene->textures[scene->textures.size() - 2]->get());
          glUniform1i(
              glGetUniformLocation(_geometryMaterial->getProgram()->get(),
                                   "gMetallicRoughness"),
              2);
          glActiveTexture(GL_TEXTURE3);
          glBindTexture(GL_TEXTURE_2D,
                        scene->textures[scene->textures.size() - 2]->get());
          glUniform1i(glGetUniformLocation(
                          _geometryMaterial->getProgram()->get(), "gOcclusion"),
                      3);
          glActiveTexture(GL_TEXTURE4);
          glBindTexture(GL_TEXTURE_2D,
                        scene->textures[scene->textures.size() - 2]->get());
          glUniform1i(glGetUniformLocation(
                          _geometryMaterial->getProgram()->get(), "gEmission"),
                      4);

          // Set default material factors
          glUniform4f(
              glGetUniformLocation(_geometryMaterial->getProgram()->get(),
                                   "gBaseColorFactor"),
              1.0f,
              1.0f,
              1.0f,
              1.0f);
          glUniform1f(
              glGetUniformLocation(_geometryMaterial->getProgram()->get(),
                                   "gMetallicFactor"),
              0.0f);
          glUniform1f(
              glGetUniformLocation(_geometryMaterial->getProgram()->get(),
                                   "gRoughnessFactor"),
              0.5f);
          glUniform1f(
              glGetUniformLocation(_geometryMaterial->getProgram()->get(),
                                   "gNormalScale"),
              1.0f);
          glUniform1f(
              glGetUniformLocation(_geometryMaterial->getProgram()->get(),
                                   "gOcclusionStrength"),
              1.0f);
          glUniform3f(
              glGetUniformLocation(_geometryMaterial->getProgram()->get(),
                                   "gEmissionFactor"),
              0.0f,
              0.0f,
              0.0f);
        }

        for (auto &prim : scene->meshes[draw.index]) {
          prim.mesh->draw();
        }
      }
    }
  }

  void SSAOPass() {
    _ssaoMaterial->gPositionMap = _positionBuffer.get();
    _ssaoMaterial->gNormalMap = _normalBuffer.get();
    _ssaoMaterial->gProj = _projection;
    _ssaoMaterial->use();
    _fullScreenTriangle->draw();
  }

  void BlurPass() {
    _blurMaterial->gColorMap = _aoBuffer.get();
    _blurMaterial->use();
    _fullScreenTriangle->draw();
  }

  void LightingPass() {
    _lightingMaterial->gAOMap = _blurBuffer.get();
    _lightingMaterial->gScreenSize =
        glm::vec2(_screen_fb_width, _screen_fb_height);
    _lightingMaterial->gNormalMap = _normalBuffer.get();
    _lightingMaterial->gAlbedoMap = _albedoBuffer.get();
    _lightingMaterial->gPositionMap = _positionBuffer.get();

    for (size_t sceneIdx = 0; sceneIdx < _scenes.size(); ++sceneIdx) {
      auto &scene = _scenes[sceneIdx];
      for (auto &draw : scene->draws) {
        // Apply scene-specific transforms
        glm::mat4 sceneTransform = draw.transform;

        // For fantasy_interior_kit (scene), adjust position
        if (sceneIdx == 0) {
          // Scale the scene to 2.0x (double size)
          glm::mat4 sceneScale = glm::scale(glm::identity<glm::mat4>(),
                                            glm::vec3(2.0f, 2.0f, 2.0f));
          glm::mat4 sceneTranslate = glm::translate(
              glm::identity<glm::mat4>(), glm::vec3(0.0f, 0.0f, 0.0f));
          sceneTransform = sceneTranslate * sceneScale * draw.transform;
        }

        // For elaina (character), position relative to player
        if (sceneIdx == 1) { // elaina is the second model
          // Scale down the character
          glm::mat4 characterScale = glm::scale(glm::identity<glm::mat4>(),
                                                glm::vec3(0.3f, 0.3f, 0.3f));
          // Rotate to make character stand upright (rotate 90 degrees around X
          // axis to flip upside down)
          glm::mat4 characterRotate = glm::rotate(glm::identity<glm::mat4>(),
                                                  glm::radians(90.0f),
                                                  glm::vec3(1.0f, 0.0f, 0.0f));
          // Character rotates with camera (face the direction camera is
          // looking)
          float characterYaw = getCharacterRotation();
          glm::mat4 characterRotateY = glm::rotate(glm::identity<glm::mat4>(),
                                                   glm::radians(characterYaw),
                                                   glm::vec3(0.0f, 1.0f, 0.0f));
          // Get animation transform (walking animation)
          glm::mat4 characterAnim = getCharacterAnimationTransform();
          // Position elaina at player position (character is the player)
          glm::mat4 characterTranslate =
              glm::translate(glm::identity<glm::mat4>(),
                           _playerPosition + glm::vec3(0.0f, 0.0f, 0.0f));
          sceneTransform = characterTranslate * characterRotateY *
                           characterRotate * characterAnim * characterScale *
                           draw.transform;
        }

        // 添加魔法阵渲染逻辑（与GeometryPass完全相同）
        if (sceneIdx == 2) { // magic_ring is the third model
          if (_showMagicRing) {
            // 显示魔法阵：应用特殊变换
            // 缩小魔法阵到合适大小
            glm::mat4 ringScale = glm::scale(glm::identity<glm::mat4>(),
                                            glm::vec3(0.5f, 0.5f, 0.5f));

            // 旋转魔法阵使其水平放置在地面上
            glm::mat4 ringRotate = glm::rotate(glm::identity<glm::mat4>(),
                                              glm::radians(90.0f),
                                              glm::vec3(0.0f, 1.0f, 0.0f));

            // 添加旋转动画效果（围绕Y轴缓慢旋转）
            static float ringRotationAngle = 0.0f;
            ringRotationAngle += 0.01f; // 每帧增加一点旋转
            if (ringRotationAngle > 360.0f) ringRotationAngle -= 360.0f;

            glm::mat4 ringAnimRotate = glm::rotate(glm::identity<glm::mat4>(),
                                                  ringRotationAngle,
                                                  glm::vec3(0.0f, 1.0f, 0.0f));

            // 添加上下浮动动画
            float floatOffset = sin(ringRotationAngle * 2.0f) * 0.05f; // 轻微浮动

            // 将魔法阵放置在角色脚下（稍微下沉到地面以下一点）
            glm::vec3 ringPosition = _playerPosition;
            ringPosition.y = _playerPosition.y - 0.8f + floatOffset; // 相对于玩家位置，稍微下沉 + 浮动

            glm::mat4 ringTranslate = glm::translate(glm::identity<glm::mat4>(),
                                                    ringPosition);

            // 组合所有变换：位置 -> 动画旋转 -> 水平旋转 -> 缩放 -> 原始变换
            sceneTransform = ringTranslate * ringAnimRotate * ringRotate *
                           ringScale * draw.transform;
          } else {
            // 不显示魔法阵：跳过渲染
            continue;
          }
        }

        // 添加火球渲染逻辑
        if (sceneIdx == 3) { // fireball is the fourth model
          // 如果没有活跃的火球效果，跳过渲染
          if (_fireballEffects.empty()) {
            continue;
          }

          // 为每个活跃的火球效果渲染
          for (const auto& fireball : _fireballEffects) {
            // 缩小火球到合适大小
            glm::mat4 fireballScale = glm::scale(glm::identity<glm::mat4>(),
                                                glm::vec3(0.3f, 0.3f, 0.3f));

            // 添加旋转动画（基于生命周期）
            float fireballRotationAngle = fireball.lifetime * 2.0f; // 旋转速度与时间相关

            glm::mat4 fireballRotate = glm::rotate(glm::identity<glm::mat4>(),
                                                  fireballRotationAngle,
                                                  glm::vec3(0.0f, 1.0f, 0.0f));

            glm::mat4 fireballTranslate = glm::translate(glm::identity<glm::mat4>(),
                                                        fireball.position);

            // 组合变换：位置 -> 旋转 -> 缩放 -> 原始变换
            sceneTransform = fireballTranslate * fireballRotate * fireballScale * draw.transform;

            // 渲染这个火球
            _lightingMaterial->gWVP = _projection * _view * sceneTransform;
            _lightingMaterial->gWV = _view * sceneTransform;
            _lightingMaterial->gWorld = sceneTransform;
            _lightingMaterial->use();

            // Set material textures
            int materialIndex = scene->meshes[draw.index][0].material;
            if (materialIndex >= 0 && materialIndex < scene->materials.size()) {
              auto &mat = *scene->materials[materialIndex];

              // 保存当前OpenGL状态
              GLboolean blendEnabled;
              GLboolean depthTestEnabled;
              glGetBooleanv(GL_BLEND, &blendEnabled);
              glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);

              _lightingMaterial->setMaterialTextures(scene.get(), mat);

              // Render the mesh
              for (auto &prim : scene->meshes[draw.index]) {
                prim.mesh->draw();
              }

              // 恢复OpenGL状态
              if (blendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
              if (depthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
            } else {
              // Use default white texture if no material
              glActiveTexture(GL_TEXTURE4);
              glBindTexture(GL_TEXTURE_2D,
                            scene->textures[scene->textures.size() - 2]->get());
              glUniform1i(glGetUniformLocation(
                              _lightingMaterial->getProgram()->get(), "gBaseColor"),
                          4);
              glActiveTexture(GL_TEXTURE5);
              glBindTexture(GL_TEXTURE_2D,
                            scene->textures[scene->textures.size() - 1]->get());
              glUniform1i(glGetUniformLocation(
                              _lightingMaterial->getProgram()->get(), "gNormal"),
                          5);
              glActiveTexture(GL_TEXTURE6);
              glBindTexture(GL_TEXTURE_2D,
                            scene->textures[scene->textures.size() - 1]->get());
              glUniform1i(glGetUniformLocation(
                              _lightingMaterial->getProgram()->get(), "gOcclusion"),
                          6);

              // Render the mesh
              for (auto &prim : scene->meshes[draw.index]) {
                prim.mesh->draw();
              }
            }
          }

          // 跳过正常的渲染循环，因为我们已经在上面渲染了所有火球
          continue;
        }

        _lightingMaterial->gWVP = _projection * _view * sceneTransform;
        _lightingMaterial->gWV = _view * sceneTransform;
        _lightingMaterial->gWorld = sceneTransform;
        _lightingMaterial->use();

        // Set material textures
        int materialIndex = scene->meshes[draw.index][0].material;
        if (materialIndex >= 0 && materialIndex < scene->materials.size()) {
          auto &mat = *scene->materials[materialIndex];

          _lightingMaterial->setMaterialTextures(scene.get(), mat);
        } else {
          // Use default white texture if no material
          glActiveTexture(GL_TEXTURE4);
          glBindTexture(GL_TEXTURE_2D,
                        scene->textures[scene->textures.size() - 2]->get());
          glUniform1i(glGetUniformLocation(
                          _lightingMaterial->getProgram()->get(), "gBaseColor"),
                      4);
          glActiveTexture(GL_TEXTURE5);
          glBindTexture(GL_TEXTURE_2D,
                        scene->textures[scene->textures.size() - 1]->get());
          glUniform1i(glGetUniformLocation(
                          _lightingMaterial->getProgram()->get(), "gNormal"),
                      5);
          glActiveTexture(GL_TEXTURE6);
          glBindTexture(GL_TEXTURE_2D,
                        scene->textures[scene->textures.size() - 2]->get());
          glUniform1i(
              glGetUniformLocation(_lightingMaterial->getProgram()->get(),
                                   "gMetallicRoughness"),
              6);
          glActiveTexture(GL_TEXTURE7);
          glBindTexture(GL_TEXTURE_2D,
                        scene->textures[scene->textures.size() - 2]->get());
          glUniform1i(glGetUniformLocation(
                          _lightingMaterial->getProgram()->get(), "gOcclusion"),
                      7);
          glActiveTexture(GL_TEXTURE8);
          glBindTexture(GL_TEXTURE_2D,
                        scene->textures[scene->textures.size() - 2]->get());
          glUniform1i(glGetUniformLocation(
                          _lightingMaterial->getProgram()->get(), "gEmission"),
                      8);

          // Set default material factors
          glUniform4f(
              glGetUniformLocation(_lightingMaterial->getProgram()->get(),
                                   "gBaseColorFactor"),
              1.0f,
              1.0f,
              1.0f,
              1.0f);
          glUniform1f(
              glGetUniformLocation(_lightingMaterial->getProgram()->get(),
                                   "gMetallicFactor"),
              0.0f);
          glUniform1f(
              glGetUniformLocation(_lightingMaterial->getProgram()->get(),
                                   "gRoughnessFactor"),
              0.5f);
          glUniform1f(
              glGetUniformLocation(_lightingMaterial->getProgram()->get(),
                                   "gNormalScale"),
              1.0f);
          glUniform1f(
              glGetUniformLocation(_lightingMaterial->getProgram()->get(),
                                   "gOcclusionStrength"),
              1.0f);
          glUniform3f(
              glGetUniformLocation(_lightingMaterial->getProgram()->get(),
                                   "gEmissionFactor"),
              0.0f,
              0.0f,
              0.0f);
        }

        for (auto &prim : scene->meshes[draw.index]) {
          prim.mesh->draw();
        }
      }
    }
  }

  void draw() {
    // sync shader type to lighting material every frame (ensures SSAO on at start)
    _lightingMaterial->gShaderType = _shaderType;

    // Update player position based on keyboard input
    static double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    float deltaTime = static_cast<float>(currentTime - lastTime);
    lastTime = currentTime;

    updatePlayerPosition(deltaTime);

    // Update camera light position to follow player
    if (_lightingMaterial->gPointLights.size() > 0) {
      _lightingMaterial->gPointLights[0].Position =
          _playerPosition + glm::vec3(0.0f, _playerHeight, 0.0f);
    }

    // Update matrices
    _view = getThirdPersonCameraView();
    float aspect = (float)_screen_fb_width / (float)_screen_fb_height;
    _projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    _model = glm::identity<glm::mat4>();

    // Update light direction
    updateLightDirection();

    // Geometry Pass - render scene to position buffer
    glBindFramebuffer(GL_FRAMEBUFFER, _gBuffer->get());
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glViewport(0, 0, _screen_fb_width, _screen_fb_height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GeometryPass();

    // SSAO Pass - calculate ambient occlusion
    // Bind the actual AO framebuffer (previously bound the texture by mistake)
    glBindFramebuffer(GL_FRAMEBUFFER, _aoFramebuffer->get());
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glViewport(0, 0, _screen_fb_width, _screen_fb_height);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    SSAOPass();

    // Blur Pass - blur the AO result
    // Bind the blur framebuffer to receive blurred AO output
    glBindFramebuffer(GL_FRAMEBUFFER, _blurFramebuffer->get());
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glViewport(0, 0, _screen_fb_width, _screen_fb_height);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    BlurPass();

    // Lighting Pass - final render with AO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glViewport(0, 0, _screen_fb_width, _screen_fb_height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    LightingPass();
  }

  void update_frame_buffer() {
    glfwGetFramebufferSize(_window, &_screen_fb_width, &_screen_fb_height);

    if (_positionBuffer != nullptr &&
        _positionBuffer->width() == _screen_fb_width &&
        _positionBuffer->height() == _screen_fb_height) {
      return;
    }

    // Create position buffer (RGB16F for view space position)
    _positionBuffer = std::make_unique<Texture2D>(nullptr,
                                                  GL_FLOAT,
                                                  _screen_fb_width,
                                                  _screen_fb_height,
                                                  GL_RGB16F,
                                                  GL_RGB);

    // Create normal buffer (RGB16F for view space normal)
    _normalBuffer = std::make_unique<Texture2D>(nullptr,
                                                GL_FLOAT,
                                                _screen_fb_width,
                                                _screen_fb_height,
                                                GL_RGB16F,
                                                GL_RGB);

    // Create albedo buffer (RGBA8 for base color)
    _albedoBuffer = std::make_unique<Texture2D>(nullptr,
                                                GL_UNSIGNED_BYTE,
                                                _screen_fb_width,
                                                _screen_fb_height,
                                                GL_RGBA8,
                                                GL_RGBA);

    // Create depth buffer
    _depthBuffer = std::make_unique<Texture2D>(nullptr,
                                               GL_UNSIGNED_INT_24_8,
                                               _screen_fb_width,
                                               _screen_fb_height,
                                               GL_DEPTH24_STENCIL8,
                                               GL_DEPTH_STENCIL);

    // Create AO buffer
    _aoBuffer = std::make_unique<Texture2D>(nullptr,
                                            GL_UNSIGNED_BYTE,
                                            _screen_fb_width,
                                            _screen_fb_height,
                                            GL_RED,
                                            GL_RED);

    // Create blur buffer
    _blurBuffer = std::make_unique<Texture2D>(nullptr,
                                              GL_UNSIGNED_BYTE,
                                              _screen_fb_width,
                                              _screen_fb_height,
                                              GL_RED,
                                              GL_RED);

    // Create G-Buffer for geometry pass (position, normal, albedo)
    Texture2D *gBufferAttachments[] = {
        _positionBuffer.get(), _normalBuffer.get(), _albedoBuffer.get()};
    _gBuffer = std::make_unique<Framebuffer>(
        gBufferAttachments, 3, _depthBuffer.get());

    // Create AO framebuffer
    Texture2D *aoAttachments[] = {_aoBuffer.get()};
    _aoFramebuffer = std::make_unique<Framebuffer>(aoAttachments, 1, nullptr);

    // Create blur framebuffer
    Texture2D *blurAttachments[] = {_blurBuffer.get()};
    _blurFramebuffer =
        std::make_unique<Framebuffer>(blurAttachments, 1, nullptr);
  }

  void update() override {
    update_frame_buffer();

    // Update fireball effects
    float deltaTime = 1.0f / 60.0f; // Assume 60 FPS for simplicity
    for (auto it = _fireballEffects.begin(); it != _fireballEffects.end();) {
      it->position += it->velocity * deltaTime;
      it->lifetime += deltaTime;

      // Remove fireball if lifetime exceeded
      if (it->lifetime >= it->maxLifetime) {
        it = _fireballEffects.erase(it);
      } else {
        ++it;
      }
    }

    draw_ui();
    draw();
  }

  // Materials
  std::unique_ptr<GeometryPassMaterial> _geometryMaterial{};
  std::unique_ptr<SSAOMaterial> _ssaoMaterial{};
  std::unique_ptr<BlurMaterial> _blurMaterial{};
  std::unique_ptr<LightingMaterial> _lightingMaterial{};

  // Framebuffers and textures
  int _screen_fb_width, _screen_fb_height;
  std::unique_ptr<Texture2D> _positionBuffer{};
  std::unique_ptr<Texture2D> _normalBuffer{};
  std::unique_ptr<Texture2D> _albedoBuffer{};
  std::unique_ptr<Texture2D> _depthBuffer{};
  std::unique_ptr<Texture2D> _aoBuffer{};
  std::unique_ptr<Texture2D> _blurBuffer{};
  std::unique_ptr<Framebuffer> _gBuffer{};
  std::unique_ptr<Framebuffer> _aoFramebuffer{};
  std::unique_ptr<Framebuffer> _blurFramebuffer{};

  // Rendering objects
  std::unique_ptr<Mesh> _fullScreenTriangle;
  std::unique_ptr<Renderer> _renderer;
  std::vector<std::unique_ptr<Gltf>> _scenes;

  // Matrices
  glm::mat4 _view{};
  glm::mat4 _projection{};
  glm::mat4 _model{};

  // UI state
  int _shaderType = 1; // Default to SSAO mode

  // Light direction control
  float _lightYaw = glm::radians(60.0f);
  float _lightPitch = glm::radians(60.0f);

  // Player state
  glm::vec3 _playerPosition{0.0f, 1.7f, 0.0f};
  float _playerHeight = 1.7f;
  float _moveSpeed = 5.0f;
  float _mouseSensitivity = 0.005f; // Increased mouse sensitivity
  bool _mouseLookEnabled = true;    // Toggle mouse-driven camera
  bool _firstMouse = true;
  bool _keys[1024] = {false}; // GLFW_KEY_LAST = 1024

  // Animation state
  float _animationTime = 0.0f;
  bool _isMoving = false;
  bool _isMovingVertical = false;
  glm::vec3 _lastPosition{0.0f, 0.0f, 0.0f};

  // Magic ring control
  bool _showMagicRing = false;

  // Fireball effects
  std::vector<FireballEffect> _fireballEffects;

  // Mouse interaction
  double _lastMouseX = 0.0;
  double _lastMouseY = 0.0;

  // Camera control
  float _cameraYaw = glm::radians(0.0f);
  float _cameraPitch = glm::radians(0.0f);
};

int main() {
  try {
    SSAOApp app{};
    app.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}