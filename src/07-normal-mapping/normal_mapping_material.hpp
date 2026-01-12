#pragma once

#include "../common/renderer.hpp"
#include "../common/shader.hpp"
#include "../common/texture.hpp"

class NormalMappingMaterial : public IMaterial {
public:
  Texture2D *diffuse_texture;
  Texture2D *normal_texture;
  glm::vec3 light_position;
  glm::vec3 light_color;
  glm::vec3 light_ambient;
  float normal_strength;

  NormalMappingMaterial();
  void use() override;

private:
  std::unique_ptr<Program> _program;
  GLint _diffuse_texture_location;
  GLint _normal_texture_location;
  GLint _light_position_location;
  GLint _light_color_location;
  GLint _light_ambient_location;
  GLint _normal_strength_location;

  std::unique_ptr<Buffer> _transform_buffer;
  std::unique_ptr<Buffer> _params_buffer;
};