#version 330 core
in vec2 vUV;
out vec4 FragColor;

// G-buffer textures
uniform sampler2D uAlbedoTex;
uniform sampler2D uNormalTex;
uniform sampler2D uDepthTex;
uniform sampler2D uSSAOTex;

// Lighting uniforms
uniform vec3 uDirectionalLightDir;
uniform vec3 uDirectionalLightColor;
uniform float uDirectionalLightIntensity;

uniform vec3 uPointLightPositions[8];
uniform vec3 uPointLightColors[8];
uniform float uPointLightIntensities[8];
uniform float uPointLightRadii[8];
uniform int uNumPointLights;

// Camera uniforms
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uInvView;
uniform mat4 uInvProj;
uniform float uNear;
uniform float uFar;

vec3 reconstructWorldPos(float depth, vec2 uv) {
    // Convert UV to NDC
    vec4 ndcPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    // Convert to view space
    vec4 viewPos = uInvProj * ndcPos;
    viewPos /= viewPos.w;
    // Convert to world space
    vec4 worldPos = uInvView * viewPos;
    return worldPos.xyz;
}

void main() {
    vec3 albedo = texture(uAlbedoTex, vUV).rgb;
    vec3 normal = texture(uNormalTex, vUV).rgb * 2.0 - 1.0; // Unpack normal
    float depth = texture(uDepthTex, vUV).r;

    // Skip lighting calculation for skybox/background
    if (depth >= 1.0) {
        FragColor = vec4(albedo, 1.0);
        return;
    }

    vec3 worldPos = reconstructWorldPos(depth, vUV);
    vec3 viewDir = normalize(-worldPos); // Camera is at origin in view space

    vec3 finalColor = vec3(0.0);

    // Directional light
    float dirLightDot = max(dot(normal, -uDirectionalLightDir), 0.0);
    vec3 dirLightContribution = uDirectionalLightColor * uDirectionalLightIntensity * dirLightDot;
    finalColor += albedo * dirLightContribution * 0.5; // Ambient + diffuse

    // Point lights
    for (int i = 0; i < uNumPointLights; ++i) {
        vec3 lightPos = uPointLightPositions[i];
        vec3 lightDir = normalize(lightPos - worldPos);
        float distance = length(lightPos - worldPos);
        float attenuation = 1.0 / (1.0 + distance * distance / (uPointLightRadii[i] * uPointLightRadii[i]));

        float lightDot = max(dot(normal, lightDir), 0.0);
        vec3 pointLightContribution = uPointLightColors[i] * uPointLightIntensities[i] * lightDot * attenuation;
        finalColor += albedo * pointLightContribution;
    }

    // Add some ambient lighting with SSAO
    float ssao = texture(uSSAOTex, vUV).r;
    finalColor += albedo * 0.1 * ssao;

    // Simple tone mapping and gamma correction
    finalColor = finalColor / (finalColor + vec3(1.0)); // Reinhard tone mapping
    finalColor = pow(finalColor, vec3(1.0/2.2)); // Gamma correction

    FragColor = vec4(finalColor, 1.0);
}
