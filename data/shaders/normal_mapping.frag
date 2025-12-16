#version 330 core

in vec3 position_vs;
in vec3 normal_vs;
in vec3 tangent_vs;
in vec3 bitangent_vs;
in vec2 uv0_vs;

layout(std140) uniform Params {
  vec3 light_position;
  float normal_strength;
  vec3 light_color;
  float padding1;
  vec3 light_ambient;
  float padding2;
};

uniform sampler2D diffuse_texture;
uniform sampler2D normal_texture;

layout(location = 0) out vec4 frag_color_out;

vec3 safe_normalize(vec3 v) {
  return dot(v, v) == 0 ? v : normalize(v);
}

vec3 decode_normal_ts() {
  vec3 normal = texture(normal_texture, uv0_vs).xyz * 2.0 - 1.0;
  return safe_normalize(normal * vec3(normal_strength, normal_strength, 1.0));
}

vec3 get_normal_vs() {
  vec3 normal_ts = decode_normal_ts();
  // avoid NAN when tangent is not present
  return normal_ts.x * safe_normalize(tangent_vs) +
         normal_ts.y * safe_normalize(bitangent_vs) +
         normal_ts.z * safe_normalize(normal_vs);
}

void main() {
  vec3 n_vs = get_normal_vs();
  vec3 l_vs = light_position;
  vec3 v_vs = -normalize(position_vs);
  n_vs = faceforward(n_vs, -v_vs, n_vs);

  // 获取漫反射颜色
  vec4 diffuse_color = texture(diffuse_texture, uv0_vs);
  
  // 计算光照
  vec3 light_dir = normalize(l_vs - position_vs);
  vec3 view_dir = normalize(v_vs);
  vec3 reflect_dir = reflect(-light_dir, n_vs);
  
  // 环境光
  vec3 ambient = light_ambient * diffuse_color.rgb;
  
  // 漫反射
  float diff = max(dot(n_vs, light_dir), 0.0);
  vec3 diffuse = light_color * diff * diffuse_color.rgb;
  
  // 镜面反射
  float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0);
  vec3 specular = light_color * spec * vec3(0.5);
  
  vec3 result = ambient + diffuse + specular;
  frag_color_out = vec4(result, diffuse_color.a);
}