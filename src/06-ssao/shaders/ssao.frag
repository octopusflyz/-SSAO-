#version 330 core
in vec2 vUV;
out float FragColor;

uniform sampler2D uDepthTex;

void main() {
    float depth = texture(uDepthTex, vUV).r;

    // Skip calculation for background
    if (depth >= 1.0) {
        FragColor = 1.0;
        return;
    }

    float occlusion = 0.0;
    int samples = 0;

    // Simple screen-space sampling around current pixel
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            if (x == 0 && y == 0) continue;

            vec2 offset = vec2(float(x), float(y)) * 0.005;
            vec2 sampleUV = vUV + offset;

            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
                continue;
            }

            float sampleDepth = texture(uDepthTex, sampleUV).r;

            if (sampleDepth > depth + 0.001) {
                occlusion += 1.0;
            }

            samples++;
        }
    }

    if (samples > 0) {
        occlusion = occlusion / float(samples);
        occlusion = 1.0 - occlusion;
    } else {
        occlusion = 1.0;
    }

    FragColor = clamp(occlusion, 0.0, 1.0);
}
