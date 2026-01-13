#version 330 core

in vec3 position_vs;
in vec3 normal_vs;
in vec2 uv0_vs;

layout(location = 0) out vec3 PosOut;
layout(location = 1) out vec3 NormalOut;
layout(location = 2) out vec4 AlbedoOut;

uniform sampler2D gBaseColor;
uniform sampler2D gNormal;
uniform sampler2D gMetallicRoughness;
uniform sampler2D gOcclusion;
uniform sampler2D gEmission;

uniform vec4 gBaseColorFactor;
uniform float gMetallicFactor;
uniform float gRoughnessFactor;
uniform float gNormalScale;
uniform float gOcclusionStrength;
uniform vec3 gEmissionFactor;

vec3 srgb_to_linear(vec3 srgb) {
  return pow(srgb, vec3(2.2));
}

vec3 decode_normal_ts() {
  vec3 normal = texture(gNormal, uv0_vs).xyz * 2.0 - 1.0;
  return normalize(normal * vec3(gNormalScale, gNormalScale, 1.0));
}

void main() {
  PosOut = position_vs;
  NormalOut = normalize(normal_vs);
  
  // Get base color
  vec4 baseColor = texture(gBaseColor, uv0_vs);
  vec4 linearBaseColor = vec4(srgb_to_linear(baseColor.rgb), baseColor.a) * gBaseColorFactor;
  AlbedoOut = linearBaseColor;
}