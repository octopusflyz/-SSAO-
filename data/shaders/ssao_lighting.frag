#version 330 core

in vec3 position_vs;
in vec3 normal_vs;
in vec2 uv0_vs;

out vec4 FragColor;

uniform sampler2D gAOMap;
uniform vec2 gScreenSize;
uniform int gShaderType;

struct BaseLight {
    vec3 Color;
    float AmbientIntensity;
    vec3 Direction;
    float DiffuseIntensity;
};

uniform BaseLight gLight;

vec2 CalcScreenTexCoord() {
    return gl_FragCoord.xy / gScreenSize;
}

vec4 CalcLightInternal(BaseLight Light, vec3 LightDirection, vec3 Normal) {
    vec4 AmbientColor = vec4(Light.Color * Light.AmbientIntensity, 1.0f);

    if (gShaderType == 1) { // SSAO mode
        AmbientColor *= texture(gAOMap, CalcScreenTexCoord()).r;
    }

    float DiffuseFactor = dot(Normal, -LightDirection);

    vec4 DiffuseColor = vec4(0, 0, 0, 0);

    if (DiffuseFactor > 0) {
        DiffuseColor = vec4(Light.Color * Light.DiffuseIntensity * DiffuseFactor, 1.0f);
    }

    return (AmbientColor + DiffuseColor);
}

void main() {
    vec3 Normal = normalize(normal_vs);
    vec3 LightDirection = normalize(vec3(0.5, 0.5, 0.5)); // Simple directional light
    
    vec4 TotalLight = CalcLightInternal(gLight, LightDirection, Normal);
    
    if (gShaderType == 2) { // Show only AO
        FragColor = texture(gAOMap, CalcScreenTexCoord());
    } else {
        FragColor = vec4(TotalLight.rgb, 1.0);
    }
}