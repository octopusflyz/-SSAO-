#version 330 core
in vec3 vNormal;
in vec2 vUV;
out vec4 FragColor;

void main() {
    vec3 n = normalize(vNormal);
    float nd = dot(n, normalize(vec3(0.3,1.0,0.2))) * 0.5 + 0.5;
    FragColor = vec4(vec3(nd), 1.0);
}
