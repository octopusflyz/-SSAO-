#version 330 core
in vec3 vNormalView;
in vec3 vAlbedo;
in vec2 vUV;

layout(location = 0) out vec3 gAlbedo;
layout(location = 1) out vec4 gNormal; // store as rgba to keep alignment

uniform sampler2D uAlbedoTex;
uniform int uHasAlbedoTex;

void main() {
    if (uHasAlbedoTex == 1) {
        gAlbedo = texture(uAlbedoTex, vUV).rgb;
    } else {
        gAlbedo = vAlbedo;
    }
    vec3 n = normalize(vNormalView);
    gNormal = vec4(n * 0.5 + 0.5, 1.0); // pack normals into 0..1
}
