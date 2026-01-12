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

class SSAOApp final : public Application {
public:
  SSAOApp() : Application("SSAO Tutorial", 800, 600) {}

private:
  // Mouse interaction
  void cursor_position_callback(double xpos, double ypos) override {
    if (_isMouseDragging) {
      double deltaX = xpos - _lastMouseX;
      double deltaY = ypos - _lastMouseY;

      // Update camera based on mouse movement
      _cameraYaw += deltaX * 0.01f;
      _cameraPitch += deltaY * 0.01f;

      // Clamp pitch to avoid gimbal lock
      _cameraPitch = std::max(-glm::radians(89.0f),
                              std::min(_cameraPitch, glm::radians(89.0f)));

      _lastMouseX = xpos;
      _lastMouseY = ypos;
    }
  }

  void mouse_button_callback(int button, int action, int mods) override {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      if (action == GLFW_PRESS) {
        _isMouseDragging = true;
        // Get current mouse position when button is pressed
        glfwGetCursorPos(_window, &_lastMouseX, &_lastMouseY);
      } else if (action == GLFW_RELEASE) {
        _isMouseDragging = false;
      }
    }
  }

  void scroll_callback(double xoffset, double yoffset) override {
    _cameraDistance += yoffset * 0.1f;
    _cameraDistance = std::max(0.5f, std::min(_cameraDistance, 10.0f));
  }

  glm::mat4 getCustomCameraView() const {
    glm::vec3 front;
    front.x = cos(_cameraPitch) * sin(_cameraYaw);
    front.y = sin(_cameraPitch);
    front.z = cos(_cameraPitch) * cos(_cameraYaw);
    front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    glm::vec3 position = -front * _cameraDistance;
    return glm::lookAt(position, position + front, up);
  }
  void init() override {
    _scene = std::make_unique<Gltf>("FlightHelmet/FlightHelmet.gltf");
    _renderer = std::make_unique<Renderer>();

    // Initialize materials
    _geometryMaterial = std::make_unique<GeometryPassMaterial>();
    _ssaoMaterial = std::make_unique<SSAOMaterial>();
    _blurMaterial = std::make_unique<BlurMaterial>();
    _lightingMaterial = std::make_unique<LightingMaterial>();

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
  }

  void draw_ui() {
    ImGui::Text("SSAO Tutorial");
    ImGui::Separator();

    // Camera Controls - Read-only display when using mouse
    if (ImGui::CollapsingHeader("Camera Controls")) {
      ImGui::Text("Mouse: Left button drag to rotate");
      ImGui::Text("Scroll: Zoom in/out");
      ImGui::Text("Current Camera Distance: %.2f", _cameraDistance);
      ImGui::Text("Current Camera Yaw: %.1f°", glm::degrees(_cameraYaw));
      ImGui::Text("Current Camera Pitch: %.1f°", glm::degrees(_cameraPitch));
      
      ImGui::Separator();
      ImGui::Text("Or use sliders below:");
      
      // Separate UI controls that don't interfere with mouse
      static bool useUICamera = false;
      ImGui::Checkbox("Use UI Camera Control", &useUICamera);
      
      if (useUICamera) {
        ImGui::SliderFloat("Camera Distance", &_cameraDistance, 0.5f, 10.0f);
        ImGui::SliderAngle("Camera Yaw", &_cameraYaw, 0.0f, 360.0f);
        ImGui::SliderAngle("Camera Pitch", &_cameraPitch, -89.0f, 89.0f);
      }
    }

    // SSAO Settings
    if (ImGui::CollapsingHeader("SSAO Settings")) {
      ImGui::SliderFloat(
          "Sample Radius", &_ssaoMaterial->gSampleRad, 0.1f, 2.0f);
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
    _geometryMaterial->gWVP = _projection * _view * _model;
    _geometryMaterial->gWV = _view * _model;
    _geometryMaterial->use();

    for (auto &draw : _scene->draws) {
      _geometryMaterial->gWVP = _projection * _view * draw.transform;
      _geometryMaterial->gWV = _view * draw.transform;
      _geometryMaterial->use();
      for (auto &prim : _scene->meshes[draw.index]) {
        prim.mesh->draw();
      }
    }
  }

  void SSAOPass() {
    _ssaoMaterial->gPositionMap = _positionBuffer.get();
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
    _lightingMaterial->gWVP = _projection * _view * _model;
    _lightingMaterial->gWV = _view * _model;
    _lightingMaterial->gWorld = _model;
    _lightingMaterial->gAOMap = _blurBuffer.get();
    _lightingMaterial->gScreenSize =
        glm::vec2(_screen_fb_width, _screen_fb_height);
    _lightingMaterial->use();

    for (auto &draw : _scene->draws) {
      _lightingMaterial->gWVP = _projection * _view * draw.transform;
      _lightingMaterial->gWV = _view * draw.transform;
      _lightingMaterial->gWorld = draw.transform;
      _lightingMaterial->use();
      for (auto &prim : _scene->meshes[draw.index]) {
        prim.mesh->draw();
      }
    }
  }

  void draw() {
    // Update matrices
    _view = getCustomCameraView();
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
    glBindFramebuffer(GL_FRAMEBUFFER, _aoBuffer->get());
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glViewport(0, 0, _screen_fb_width, _screen_fb_height);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    SSAOPass();

    // Blur Pass - blur the AO result
    glBindFramebuffer(GL_FRAMEBUFFER, _blurBuffer->get());
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

    // Create G-Buffer for geometry pass
    Texture2D *gBufferAttachments[] = {_positionBuffer.get()};
    _gBuffer = std::make_unique<Framebuffer>(
        gBufferAttachments, 1, _depthBuffer.get());

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
  std::unique_ptr<Texture2D> _depthBuffer{};
  std::unique_ptr<Texture2D> _aoBuffer{};
  std::unique_ptr<Texture2D> _blurBuffer{};
  std::unique_ptr<Framebuffer> _gBuffer{};
  std::unique_ptr<Framebuffer> _aoFramebuffer{};
  std::unique_ptr<Framebuffer> _blurFramebuffer{};

  // Rendering objects
  std::unique_ptr<Mesh> _fullScreenTriangle;
  std::unique_ptr<Renderer> _renderer;
  std::unique_ptr<Gltf> _scene;

  // Matrices
  glm::mat4 _view{};
  glm::mat4 _projection{};
  glm::mat4 _model{};

  // UI state
  int _shaderType = 1; // Default to SSAO mode

  // Light direction control
  float _lightYaw = glm::radians(60.0f);
  float _lightPitch = glm::radians(60.0f);

  // Mouse interaction
  bool _isMouseDragging = false;
  double _lastMouseX = 0.0;
  double _lastMouseY = 0.0;
  double xpos = 0.0;
  double ypos = 0.0;

  // Camera control
  float _cameraYaw = glm::radians(60.0f);
  float _cameraPitch = glm::radians(60.0f);
  float _cameraDistance = 3.0f;
};

int main() {
  try {
    SSAOApp app{};
    app.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}