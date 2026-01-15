#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uAlbedoColor;

out vec3 vNormalView;
out vec3 vAlbedo;
out vec2 vUV;

void main() {
    vec4 viewPos = uView * uModel * vec4(aPos, 1.0);
    gl_Position = uProj * viewPos;
    vNormalView = mat3(uView * uModel) * aNormal; // view-space normal
    vAlbedo = uAlbedoColor; // Use uniform color
    vUV = aUV;
}
