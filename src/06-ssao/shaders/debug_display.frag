#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uAlbedo;
uniform sampler2D uNormal;
uniform sampler2D uDepth;
uniform int uMode; // 0 = albedo, 1 = normal, 2 = depth
uniform float uNear;
uniform float uFar;

float LinearizeDepth(float d) {
    // assuming depth stored in [0,1], reconstruct view-space linear depth (approx)
    float z = d * 2.0 - 1.0; // NDC
    float linear = (2.0 * uNear * uFar) / (uFar + uNear - z * (uFar - uNear));
    return linear;
}

void main() {
    if (uMode == 0) {
        vec3 a = texture(uAlbedo, vUV).rgb;
        FragColor = vec4(a, 1.0);
    } else if (uMode == 1) {
        vec3 n = texture(uNormal, vUV).rgb;
        vec3 dec = n * 2.0 - 1.0; // unpack
        FragColor = vec4(normalize(dec) * 0.5 + 0.5, 1.0);
    } else if (uMode == 2) {
        float d = texture(uDepth, vUV).r;
        float lin = LinearizeDepth(d);
        float v = clamp(lin / uFar, 0.0, 1.0);
        FragColor = vec4(vec3(v), 1.0);
    } else {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0);
    }
}
