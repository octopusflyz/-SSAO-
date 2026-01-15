#version 330 core

in vec3 position_vs;
in vec3 normal_vs;
in vec2 uv0_vs;

out vec4 FragColor;

uniform sampler2D gAOMap;
uniform sampler2D gNormalMap;
uniform sampler2D gAlbedoMap;
uniform sampler2D gPositionMap;
uniform vec2 gScreenSize;
uniform int gShaderType;

// Material textures
uniform sampler2D gBaseColor;
uniform sampler2D gNormal;
uniform sampler2D gMetallicRoughness;
uniform sampler2D gOcclusion;
uniform sampler2D gEmission;

// Material factors
uniform vec4 gBaseColorFactor;
uniform float gMetallicFactor;
uniform float gRoughnessFactor;
uniform float gNormalScale;
uniform float gOcclusionStrength;
uniform vec3 gEmissionFactor;

struct BaseLight {
    vec3 Color;
    float AmbientIntensity;
    vec3 Direction;
    float DiffuseIntensity;
};

struct PointLight {
    vec3 Position;
    vec3 Color;
    float Intensity;
    float Radius;
};

uniform BaseLight gLight;
uniform PointLight gPointLights[4];
uniform int gNumPointLights;

vec2 CalcScreenTexCoord() {
    return gl_FragCoord.xy / gScreenSize;
}

vec3 srgb_to_linear(vec3 srgb) {
    return pow(srgb, vec3(2.2));
}

vec3 decode_normal_ts() {
    vec3 normal = texture(gNormal, uv0_vs).xyz * 2.0 - 1.0;
    return normalize(normal * vec3(gNormalScale, gNormalScale, 1.0));
}

vec3 get_normal_vs() {
    vec3 normal_ts = decode_normal_ts();
    vec3 normal_vs = normalize(normal_vs);
    vec3 tangent_vs = normalize(vec3(normal_vs.z, 0.0, -normal_vs.x));
    vec3 bitangent_vs = cross(normal_vs, tangent_vs);
    return normal_ts.x * tangent_vs + normal_ts.y * bitangent_vs + normal_ts.z * normal_vs;
}

float PI = 3.14159;

float square(float v) {
    return v * v;
}

float D_GGX(float NoH, float roughness) {
    float a2 = square(roughness);
    float c2 = square(NoH);
    return a2 / (PI * square(a2 * c2 + 1.0 - c2));
}

float V1_SmithGGX(float NoV, float roughness) {
    float a2 = square(roughness);
    return 1.0 / (NoV + sqrt(square(NoV) + a2 * (1.0 - square(NoV))));
}

float V_SmithGGX(float NoV, float NoL, float roughness) {
    return V1_SmithGGX(NoV, roughness) * V1_SmithGGX(NoL, roughness);
}

vec3 F_Schlick(float LoH, vec3 f0) {
    float f = pow(1.0 - LoH, 5.0);
    return f + f0 * (1.0 - f);
}

struct BRDF {
    vec3 base_color;
    float metallic;
    float perceptual_roughness;
};

float min_reflectivity = 0.04;

float one_minus_reflectivity(BRDF brdf) {
    return (1.0 - min_reflectivity) * (1.0 - brdf.metallic);
}

vec3 get_reflection(BRDF brdf) {
    return mix(vec3(min_reflectivity), brdf.base_color, brdf.metallic);
}

vec3 specular_BRDF(BRDF brdf, vec3 n, vec3 v, vec3 l) {
    vec3 h = normalize(v + l);

    float NoV = clamp(abs(dot(n, v)), 0.0, 1.0);
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);

    vec3 f0 = get_reflection(brdf);

    float roughness = brdf.perceptual_roughness * brdf.perceptual_roughness;
    float D = D_GGX(NoH, roughness);
    vec3 F = F_Schlick(LoH, f0);
    float V = V_SmithGGX(NoV, NoL, roughness);

    return max(D * V * F, vec3(0.0));
}

vec3 diffuse_BRDF(BRDF brdf) {
    return brdf.base_color * one_minus_reflectivity(brdf) / PI;
}

vec3 calculatePointLight(PointLight light, vec3 pos, vec3 normal, vec3 viewDir, BRDF brdf) {
    vec3 lightDir = light.Position - pos;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);
    
    // Reduced attenuation for better light spread
    float attenuation = 1.0 / (1.0 + 0.02 * distance + 0.005 * distance * distance);
    attenuation = smoothstep(light.Radius, 0.0, distance) * attenuation;
    
    float NoL = clamp(dot(normal, lightDir), 0.0, 1.0);
    
    vec3 fr = specular_BRDF(brdf, normal, viewDir, lightDir);
    vec3 fd = diffuse_BRDF(brdf);
    
    vec3 lighting = (fr + fd) * light.Color * light.Intensity * NoL * attenuation;
    return lighting;
}

void main() {
    vec2 screenUV = CalcScreenTexCoord();
    
    // Get G-Buffer data
    vec3 gBufferPos = texture(gPositionMap, screenUV).xyz;
    vec3 gBufferNormal = normalize(texture(gNormalMap, screenUV).xyz);
    vec4 gBufferAlbedo = texture(gAlbedoMap, screenUV);
    
    // Skip background pixels
    if (length(gBufferPos) > 100.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // Get material data
    vec4 baseColor = texture(gBaseColor, uv0_vs);
    vec4 linearBaseColor = vec4(srgb_to_linear(baseColor.rgb), baseColor.a) * gBaseColorFactor;
    vec4 metallicRoughness = texture(gMetallicRoughness, uv0_vs);
    
    BRDF brdf;
    brdf.base_color = linearBaseColor.rgb * linearBaseColor.a;
    brdf.metallic = metallicRoughness.b * gMetallicFactor;
    brdf.perceptual_roughness = metallicRoughness.g * gRoughnessFactor;
    
    vec3 n_vs = get_normal_vs();
    vec3 l_vs = normalize(gLight.Direction);
    vec3 v_vs = -normalize(position_vs);
    n_vs = faceforward(n_vs, -v_vs, n_vs);
    
    float NoV = clamp(dot(n_vs, v_vs), 0.0, 1.0);
    float NoL = clamp(dot(n_vs, l_vs), 0.0, 1.0);
    
    vec3 fr = specular_BRDF(brdf, n_vs, v_vs, l_vs);
    vec3 fd = diffuse_BRDF(brdf);
    
    vec3 directional = (fr + fd) * gLight.Color * gLight.DiffuseIntensity * NoL;
    vec3 ambient = gLight.Color * gLight.AmbientIntensity * fd;
    
    // Add point lights
    vec3 pointLighting = vec3(0.0);
    for (int i = 0; i < gNumPointLights; i++) {
        pointLighting += calculatePointLight(gPointLights[i], gBufferPos, n_vs, v_vs, brdf);
    }
    
    // Apply SSAO: ambient全受影响，直射/点光部分受影响以增强可见度
    if (gShaderType == 1) {
        float ao = texture(gAOMap, screenUV).r;
        ambient *= ao;
        directional *= mix(1.0, ao, 0.35);
        pointLighting *= mix(1.0, ao, 0.35);
    }
    
    // Apply occlusion texture: 只影响环境/间接，不压制直接光
    float occlusion = texture(gOcclusion, uv0_vs).r;
    ambient *= mix(1.0, occlusion, clamp(gOcclusionStrength, 0.0, 1.0));
    
    // Apply emission
    vec3 emission = srgb_to_linear(texture(gEmission, uv0_vs).xyz) * gEmissionFactor;
    
    vec3 finalColor = directional + ambient + pointLighting + emission;
    
    if (gShaderType == 2) { // Show only AO
        float ao_vis = texture(gAOMap, screenUV).r;
        FragColor = vec4(vec3(ao_vis), 1.0);
    } else {
        FragColor = vec4(finalColor, linearBaseColor.a);
    }
}