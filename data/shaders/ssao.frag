#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D gPositionMap;
uniform sampler2D gNormalMap;
uniform float gSampleRad;
uniform mat4 gProj;

const int MAX_KERNEL_SIZE = 64;
uniform vec3 gKernel[MAX_KERNEL_SIZE];

vec3 getNormal(vec2 uv) {
    return normalize(texture(gNormalMap, uv).xyz);
}
// 生成伪随机向量用于旋转采样核心
vec3 getRandomVec(vec2 uv) {
    float r = fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
    float r2 = fract(sin(dot(uv, vec2(39.3467, 11.135))) * 24634.6345);
    return normalize(vec3(r * 2.0 - 1.0, r2 * 2.0 - 1.0, 0.0));
}

void main() {
    vec3 Pos = texture(gPositionMap, TexCoord).xyz;
    vec3 Normal = getNormal(TexCoord);
    
    // 背景检测：仅剔除无效(零)位置，避免远处被当成白底
    if (dot(Pos, Pos) < 1e-8) { FragColor = vec4(1.0); return; }

    // 构建TBN矩阵，用随机向量旋转采样核心
    vec3 randomVec = getRandomVec(TexCoord);
    vec3 tangent = normalize(randomVec - Normal * dot(randomVec, Normal));
    vec3 bitangent = cross(Normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, Normal);

    float occlusion = 0.0;
    int validSamples = 0;

    for (int i = 0; i < MAX_KERNEL_SIZE; i++) {
        // 将采样核心从切线空间转换到view space
        vec3 samplePos = Pos + TBN * gKernel[i] * gSampleRad;
        
        // 投影到屏幕空间
        vec4 offset = gProj * vec4(samplePos, 1.0);
        if (offset.w <= 0.0) continue;
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;
        
        // 边界检查
        if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0) continue;

        validSamples++;

        // 读取该位置的实际深度
        float sampleDepth = texture(gPositionMap, offset.xy).z;
        
        // 范围检查：放宽权重让遮挡在小半径下也可见
        float rangeCheck = smoothstep(0.0, 1.0, (gSampleRad * 2.0) / max(1e-4, abs(Pos.z - sampleDepth)));
        
        // 深度比较：OpenGL view space中z为负，z更大(不那么负)表示更近
        float bias = 0.008;
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    if (validSamples == 0) { FragColor = vec4(1.0); return; }

    float AO = 1.0 - (occlusion / float(validSamples));
    
    // 输出到单通道纹理
    FragColor = vec4(AO, AO, AO, 1.0);
}