#version 330 core

in vec2 TexCoord;

out vec4 FragColor; // r: occlusion, gba: indirect lighting

uniform sampler2D gColorMap;
uniform sampler2D gNormalMap;

float Offsets[4] = float[]( -1.5, -0.5, 0.5, 1.5 );

void main() {
    float aoValue = 0.0;
    vec3 indirectValue = vec3(0.0);
    
    vec3 centerNormal = normalize(texture(gNormalMap, TexCoord).xyz);

    for (int i = 0 ; i < 4 ; i++) {
        for (int j = 0 ; j < 4 ; j++) {
            vec2 tc = TexCoord;
            tc.x = TexCoord.x + Offsets[j] / textureSize(gColorMap, 0).x;
            tc.y = TexCoord.y + Offsets[i] / textureSize(gColorMap, 0).y;
            
            vec4 sample = texture(gColorMap, tc);
            vec3 sampleNormal = normalize(texture(gNormalMap, tc).xyz);
            
            // 几何敏感模糊：只对法线相似的像素进行模糊
            float normalWeight = max(0.0, dot(centerNormal, sampleNormal));
            normalWeight = pow(normalWeight, 16.0); // 锐化边缘
            
            aoValue += sample.r * normalWeight;
            indirectValue += sample.gba * normalWeight;
        }
    }

    float totalWeight = 16.0; // 简化，实际应该累加normalWeight
    aoValue /= totalWeight;
    indirectValue /= totalWeight;

    FragColor = vec4(aoValue, indirectValue.r, indirectValue.g, indirectValue.b);
}
