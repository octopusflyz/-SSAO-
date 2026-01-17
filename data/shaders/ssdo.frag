#version 330 core

in vec2 TexCoord;

out vec4 FragColor; // r: occlusion, gba: indirect lighting

uniform sampler2D gPositionMap;
uniform sampler2D gNormalMap;
uniform sampler2D gAlbedoMap;
uniform float gSampleRad;
uniform mat4 gProj;
uniform mat4 gView;

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
    vec3 Albedo = texture(gAlbedoMap, TexCoord).rgb;
    
    // 背景检测
    if (dot(Pos, Pos) < 1e-8) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        return;
    }

    // 构建TBN矩阵，用随机向量旋转采样核心
    vec3 randomVec = getRandomVec(TexCoord);
    vec3 tangent = normalize(randomVec - Normal * dot(randomVec, Normal));
    vec3 bitangent = cross(Normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, Normal);

    float occlusion = 0.0;
    vec3 indirectLight = vec3(0.0);
    int validSamples = 0;
    
    // 每个采样点对应的面积（用于间接光照）
    float sampleArea = 3.14159 * gSampleRad * gSampleRad / float(MAX_KERNEL_SIZE);
    // 间接光照强度控制
    float indirectStrength = 2.0;

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

        // 读取该位置的实际深度和法线
        float sampleDepth = texture(gPositionMap, offset.xy).z;
        vec3 sampleNormal = getNormal(offset.xy);
        vec3 sampleAlbedo = texture(gAlbedoMap, offset.xy).rgb;
        
        // 计算采样方向
        vec3 sampleDir = normalize(samplePos - Pos);
        
        // 范围检查
        float rangeCheck = smoothstep(0.0, 1.0, (gSampleRad * 2.0) / max(1e-4, abs(Pos.z - sampleDepth)));
        
        // 深度比较：判断是否被遮挡
        float bias = 0.008;
        bool isOccluded = (sampleDepth >= samplePos.z + bias);
        
        // 计算遮挡值
        occlusion += (isOccluded ? 1.0 : 0.0) * rangeCheck;
        
        // 计算间接光照（从被遮挡的采样点反弹）
        if (isOccluded) {
            // 检查采样点是否背向当前点
            float senderCosTheta = max(0.0, dot(sampleNormal, -sampleDir));
            float receiverCosTheta = max(0.0, dot(Normal, sampleDir));
            
            // 只考虑正面朝向的采样点
            if (senderCosTheta > 0.0 && receiverCosTheta > 0.0) {
                float distance = length(samplePos - Pos);
                float distanceSquared = max(1.0, distance * distance);
                
                // 形状因子近似
                float formFactor = sampleArea * senderCosTheta * receiverCosTheta / distanceSquared;
                
                // 间接光照贡献
                indirectLight += sampleAlbedo * formFactor * rangeCheck;
            }
        }
    }

    if (validSamples == 0) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        return;
    }

    float AO = 1.0 - (occlusion / float(validSamples));
    indirectLight = indirectLight * indirectStrength / float(validSamples);
    
    // 输出：r通道为遮挡值，gba为间接光照颜色
    FragColor = vec4(AO, indirectLight.r, indirectLight.g, indirectLight.b);
}
