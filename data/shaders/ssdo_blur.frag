#version 330 core

in vec2 TexCoord;

out vec4 FragColor; // r: occlusion, gba: indirect lighting

uniform sampler2D gColorMap;
uniform sampler2D gNormalMap;

float Offsets[4] = float[]( -1.5, -0.5, 0.5, 1.5 );

void main() {
    float aoValue = 0.0;
    vec3 indirectValue = vec3(0.0);

    for (int i = 0 ; i < 4 ; i++) {
        for (int j = 0 ; j < 4 ; j++) {
            vec2 tc = TexCoord;
            tc.x = TexCoord.x + Offsets[j] / textureSize(gColorMap, 0).x;
            tc.y = TexCoord.y + Offsets[i] / textureSize(gColorMap, 0).y;
            
            vec4 sample = texture(gColorMap, tc);
            
            aoValue += sample.r;
            indirectValue += sample.gba;
        }
    }

    float totalWeight = 16.0;
    aoValue /= totalWeight;
    indirectValue /= totalWeight;
    
    // 大幅增强间接光照强度，确保可见
    indirectValue *= 10.0;
    
    // 不使用fallback，让间接光照保持原样
    // 这样可以更好地调试问题

    FragColor = vec4(aoValue, indirectValue.r, indirectValue.g, indirectValue.b);
}
