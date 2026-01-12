#version 330 core

layout(location = 0) in vec3 position_os;
layout(location = 1) in vec3 normal_os;
layout(location = 2) in vec4 tangent_os;
layout(location = 3) in vec2 uv0_os;

out vec3 position_vs;
out vec3 normal_vs;
out vec2 uv0_vs;

uniform mat4 gWVP;
uniform mat4 gWV;

vec3 transform_normal(mat4 mat_inverse, vec3 n) {
  return vec3(dot(mat_inverse[0].xyz, n),
              dot(mat_inverse[1].xyz, n),
              dot(mat_inverse[2].xyz, n));
}

vec3 safe_normalize(vec3 v) {
  return dot(v, v) == 0 ? v : normalize(v);
}

void main() {
  vec4 position_ws = gWV * vec4(position_os, 1.0);
  position_vs = position_ws.xyz;
  
  mat4 inverseWV = inverse(gWV);
  normal_vs = safe_normalize(transform_normal(inverseWV, normal_os));
  
  uv0_vs = uv0_os;

  gl_Position = gWVP * vec4(position_os, 1.0);
}