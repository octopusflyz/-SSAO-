#include "../common/application.hpp"
#include "../common/mesh.hpp"
#include "../common/profile.h"
#include "../common/renderer.hpp"
#include "../common/shader.hpp"
#include "../common/texture.hpp"
#include "../common/utils.hpp"
#include "normal_mapping_material.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <iostream>
#include <sstream>
#include <vector>

class NormalMappingApp final : public Application {
public:
  NormalMappingApp() : Application("Normal Mapping", 800, 600) {}

private:
  void init() override {
    _camera = std::make_unique<ModelViewerCamera>();
    _renderer = std::make_unique<Renderer>();
    _material = std::make_unique<NormalMappingMaterial>();
    
    // 加载纹理
    _diffuse_texture = std::make_unique<Texture2D>("wenlitu.bmp");
    _normal_texture = std::make_unique<Texture2D>("faxiangtu.bmp");
    
    // 创建立方体
    create_cube();
    
    // 初始化材质参数
    _material->diffuse_texture = _diffuse_texture.get();
    _material->normal_texture = _normal_texture.get();
    _material->light_position = glm::vec3(2.0f, 2.0f, 2.0f);
    _material->light_color = glm::vec3(1.0f, 1.0f, 1.0f);
    _material->light_ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    _material->normal_strength = 1.0f;
  }

  void create_cube() {
    // 立方体顶点数据
    std::vector<Mesh::Vertex> vertices = {
      // 前面 (Z+)
      {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      
      // 后面 (Z-)
      {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      
      // 左面 (X-)
      {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      
      // 右面 (X+)
      {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      
      // 底面 (Y-)
      {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      
      // 顶面 (Y+)
      {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
      {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
    };

    // 立方体索引
    std::vector<uint32_t> indices = {
      // 前面
      0, 1, 2, 2, 3, 0,
      // 后面
      4, 5, 6, 6, 7, 4,
      // 左面
      8, 9, 10, 10, 11, 8,
      // 右面
      12, 13, 14, 14, 15, 12,
      // 底面
      16, 17, 18, 18, 19, 16,
      // 顶面
      20, 21, 22, 22, 23, 20
    };

    _cube_mesh = std::make_unique<Mesh>(vertices.data(), vertices.size(), 
                                        indices.data(), indices.size());
  }

  void draw_ui() {
    int id = 0;
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
    
    if (ImGui::Button("Screen Shot")) {
      request_screen_shot();
    }
    
    if (ImGui::CollapsingHeader("Camera")) {
      ImGui::PushID(id++);
      _camera->draw_ui();
      ImGui::PopID();
    }
    
    if (ImGui::CollapsingHeader("Light")) {
      ImGui::PushID(id++);
      ImGui::SliderFloat3("Position", &_material->light_position[0], -5.0f, 5.0f);
      ImGui::ColorEdit3("Color", &_material->light_color[0]);
      ImGui::ColorEdit3("Ambient", &_material->light_ambient[0]);
      ImGui::PopID();
    }
    
    if (ImGui::CollapsingHeader("Normal Mapping")) {
      ImGui::PushID(id++);
      ImGui::SliderFloat("Normal Strength", &_material->normal_strength, 0.0f, 2.0f);
      ImGui::Checkbox("Rotate Model", &_rotate_model);
      ImGui::PopID();
    }
  }

  void draw() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    int fb_width, fb_height;
    glfwGetFramebufferSize(_window, &fb_width, &fb_height);
    glViewport(0, 0, fb_width, fb_height);

    float aspect = fb_height > 0 ? (float)fb_width / (float)fb_height : 1.0f;
    glm::mat4 view = _camera->view();
    glm::mat4 projection = _camera->projection(aspect);

    // 模型变换
    glm::mat4 model = glm::mat4(1.0f);
    if (_rotate_model) {
      model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // 设置材质参数
    _material->model = model;
    _material->view = view;
    _material->projection = projection;

    // 渲染立方体
    _material->use();
    _cube_mesh->draw();
  }

  void update() override {
    draw_ui();
    draw();
  }

  std::unique_ptr<Renderer> _renderer;
  std::unique_ptr<ModelViewerCamera> _camera;
  std::unique_ptr<NormalMappingMaterial> _material;
  std::unique_ptr<Mesh> _cube_mesh;
  std::unique_ptr<Texture2D> _diffuse_texture;
  std::unique_ptr<Texture2D> _normal_texture;
  
  bool _rotate_model = true;
};

int main() {
  try {
    NormalMappingApp app{};
    app.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}