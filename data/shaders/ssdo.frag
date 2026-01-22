#version 330 core

in vec2 TexCoord;

out vec4 FragColor; // r: occlusion, gba: indirect lighting

uniform sampler2D gPositionMap;
uniform sampler2D gNormalMap;
uniform sampler2D gAlbedoMap;
uniform float gSampleRad;
uniform mat4 gProj;
uniform mat4 gView;

// 调试：检查纹理单元是否正确绑定
// 在OpenGL中，sampler2D的纹理单元由uniform的值决定
// gPositionMap应该在单元0，gNormalMap在单元1，gAlbedoMap在单元2

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

    // 调试：检查Albedo值是否为蓝色
    // 如果Albedo是蓝色(0,0,1)，说明Albedo数据有问题
    if (Albedo.b > 0.5 && Albedo.r < 0.3 && Albedo.g < 0.3) {
        // 蓝色Albedo - 输出黄色警告
        FragColor = vec4(1.0, 1.0, 0.0, 1.0);
        return;
    }
    
    // 调试：直接输出Albedo数据
    // 输出格式是(r=AO, g=indirect.r, b=indirect.g, a=indirect.b)
    // 所以要输出Albedo，需要设置g=Albedo.r, b=Albedo.g, a=Albedo.b
    FragColor = vec4(1.0, Albedo.r, Albedo.g, Albedo.b);
    return;

    // 构建TBN矩阵用于将采样核心从切线空间转换到view space
    vec3 randomVec = getRandomVec(TexCoord);
    vec3 tangent = normalize(randomVec - Normal * dot(randomVec, Normal));
    vec3 bitangent = cross(Normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, Normal);

    float occlusion = 0.0;
    vec3 indirectLight = vec3(0.0);
    int validSamples = 0;
    int indirectSamples = 0;

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

        // 读取该位置的实际深度和法线
        float sampleDepth = texture(gPositionMap, offset.xy).z;
        
        // 背景检测：如果采样点落在背景区域（深度为0），跳过该采样点
        // 背景不应该产生遮挡或间接光照
        if (abs(sampleDepth) < 1e-6) {
            continue;
        }
        
        validSamples++;
        
        vec3 sampleNormal = getNormal(offset.xy);
        vec3 sampleAlbedo = texture(gAlbedoMap, offset.xy).rgb;
        
        // 范围检查
        float rangeCheck = smoothstep(0.0, 1.0, gSampleRad / max(1e-4, abs(Pos.z - sampleDepth)));
        
        // 深度比较：在view space中，z为负值
        // 更大的z值(不那么负)表示更靠近相机
        float bias = 0.025;
        bool isOccluded = (sampleDepth > samplePos.z + bias);
        
        // 计算遮挡值
        occlusion += (isOccluded ? 1.0 : 0.0) * rangeCheck;
        
        // SSDO间接光照计算
        // 使用实际的采样位置（从G-Buffer读取的深度重建）
        vec3 actualSamplePos = texture(gPositionMap, offset.xy).xyz;
        
        // 计算从当前点到实际采样点的方向
        vec3 toSample = normalize(actualSamplePos - Pos);
        
        // 检查当前点法线是否朝向采样点（当前点作为接收者）
        float receiverCosTheta = max(0.0, dot(Normal, toSample));
        
        // 检查采样点法线是否背向当前点（采样点作为发射者）
        float emitterCosTheta = max(0.0, dot(sampleNormal, -toSample));
        
        // 只考虑未被遮挡的采样点
        // 移除cosine条件，让更多采样点贡献间接光照
        if (!isOccluded) {
            float distance = length(actualSamplePos - Pos);
            
            // 距离衰减：使用非常温和的衰减
            float attenuation = 1.0 / (1.0 + 0.001 * distance * distance);
            
            // 间接光照贡献：使用采样点颜色，不使用cosine项
            vec3 contribution = sampleAlbedo * attenuation * rangeCheck;
            
            // 大幅增强贡献强度
            indirectLight += contribution * 100.0;
            indirectSamples++;
        }
    }

    if (validSamples == 0) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        return;
    }

    float AO = 1.0 - (occlusion / float(validSamples));
    
    // 调试：如果间接光照采样数为0，使用当前像素的Albedo
    if (indirectSamples == 0) {
        indirectLight = Albedo * 2.0;
    }
    
    // 增强间接光照：使用更宽松的clamp范围
    indirectLight = clamp(indirectLight, 0.0, 10.0);
    
    // 输出：r通道为遮挡值，gba为间接光照颜色
    FragColor = vec4(AO, indirectLight.r, indirectLight.g, indirectLight.b);
}
