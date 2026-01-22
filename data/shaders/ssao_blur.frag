#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D gColorMap;

float Offsets[4] = float[]( -1.5, -0.5, 0.5, 1.5 );

void main() {
    float aoValue = 0.0;

    for (int i = 0 ; i < 4 ; i++) {
        for (int j = 0 ; j < 4 ; j++) {
            vec2 tc = TexCoord;
            tc.x = TexCoord.x + Offsets[j] / textureSize(gColorMap, 0).x;
            tc.y = TexCoord.y + Offsets[i] / textureSize(gColorMap, 0).y;
            aoValue += texture(gColorMap, tc).r;
        }
    }

    aoValue /= 16.0;

    // 只输出到R通道，保持与输入格式一致
    FragColor = vec4(aoValue, 0.0, 0.0, 1.0);
}