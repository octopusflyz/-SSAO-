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
    
    // 背景检测：仅剔除无效(零)位置
    if (dot(Pos, Pos) < 1e-8) { 
        FragColor = vec4(1.0); 
        return; 
    }

    // 构建TBN矩阵用于将采样核心从切线空间转换到view space
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

        // 读取该位置的实际深度
        float sampleDepth = texture(gPositionMap, offset.xy).z;
        
        // 背景检测：如果采样点落在背景区域（深度为0），跳过该采样点
        // 背景不应该产生遮挡
        if (abs(sampleDepth) < 1e-6) {
            continue;
        }
        
        validSamples++;
        
        // 范围检查
        float rangeCheck = smoothstep(0.0, 1.0, gSampleRad / max(1e-4, abs(Pos.z - sampleDepth)));
        
        // 深度比较：在view space中，z为负值
        // 更大的z值(不那么负)表示更靠近相机
        // 如果采样点的实际深度比我们期望的采样位置更近，则发生遮挡
        float bias = 0.025;
        bool isOccluded = (sampleDepth > samplePos.z + bias);
        
        // 计算遮挡值
        occlusion += (isOccluded ? 1.0 : 0.0) * rangeCheck;
    }

    if (validSamples == 0) {
        FragColor = vec4(1.0);
        return;
    }

    // 计算最终的AO值
    float ao = 1.0 - (occlusion / float(validSamples));
    
    // 确保AO在合理范围
    ao = clamp(ao, 0.0, 1.0);
    
    // 输出到单通道纹理：只使用R通道存储AO值
    FragColor = vec4(ao, 0.0, 0.0, 1.0);
}
