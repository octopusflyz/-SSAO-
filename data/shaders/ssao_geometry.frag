#version 330 core

in vec3 position_vs;
in vec3 normal_vs;
in vec2 uv0_vs;

layout(location = 0) out vec3 PosOut;

void main() {
  PosOut = position_vs;
}