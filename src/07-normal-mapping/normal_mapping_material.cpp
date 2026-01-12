#include "normal_mapping_material.hpp"

namespace {
struct TransformBlock {
  glm::mat4 MV;
  glm::mat4 I_MV;
  glm::mat4 P;
};

struct ParamsBlock {
  glm::vec3 light_position;
  float normal_strength;
  glm::vec3 light_color;
  float padding1;
  glm::vec3 light_ambient;
  float padding2;
};
} // namespace

NormalMappingMaterial::NormalMappingMaterial() {
  _program = Program::create_from_files("shaders/normal_mapping.vert", 
                                        "shaders/normal_mapping.frag");

  _diffuse_texture_location = glGetUniformLocation(_program->get(), "diffuse_texture");
  _normal_texture_location = glGetUniformLocation(_program->get(), "normal_texture");
  _light_position_location = glGetUniformLocation(_program->get(), "light_position");
  _light_color_location = glGetUniformLocation(_program->get(), "light_color");
  _light_ambient_location = glGetUniformLocation(_program->get(), "light_ambient");
  _normal_strength_location = glGetUniformLocation(_program->get(), "normal_strength");

  GLuint transform_index = glGetUniformBlockIndex(_program->get(), "Transform");
  glUniformBlockBinding(_program->get(), transform_index, 0);
  GLuint params_index = glGetUniformBlockIndex(_program->get(), "Params");
  glUniformBlockBinding(_program->get(), params_index, 1);

  _transform_buffer = std::make_unique<Buffer>(nullptr, sizeof(TransformBlock));
  _params_buffer = std::make_unique<Buffer>(nullptr, sizeof(ParamsBlock));
}

void NormalMappingMaterial::use() {
  glUseProgram(_program->get());

  // 绑定纹理
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, diffuse_texture != nullptr ? diffuse_texture->get() : 0);
  glUniform1i(_diffuse_texture_location, 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, normal_texture != nullptr ? normal_texture->get() : 0);
  glUniform1i(_normal_texture_location, 1);

  // 更新变换矩阵
  TransformBlock transform_block{};
  transform_block.MV = view * model;
  transform_block.I_MV = glm::inverse(transform_block.MV);
  transform_block.P = projection;
  glBindBuffer(GL_UNIFORM_BUFFER, _transform_buffer->get());
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(TransformBlock), &transform_block);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  // 更新参数
  ParamsBlock params_block{};
  params_block.light_position = light_position;
  params_block.light_color = light_color;
  params_block.light_ambient = light_ambient;
  params_block.normal_strength = normal_strength;

  glBindBuffer(GL_UNIFORM_BUFFER, _params_buffer->get());
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ParamsBlock), &params_block);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  // 绑定uniform块
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, _transform_buffer->get());
  glBindBufferBase(GL_UNIFORM_BUFFER, 1, _params_buffer->get());
}