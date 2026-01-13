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

void main() {
    vec3 Pos = texture(gPositionMap, TexCoord).xyz;
    vec3 Normal = getNormal(TexCoord);
    
    // Skip background pixels
    if (length(Pos) > 50.0) {
        FragColor = vec4(0.0);
        return;
    }

    float AO = 0.0;

    for (int i = 0 ; i < MAX_KERNEL_SIZE ; i++) {
        // Orient sample along normal
        vec3 sampleVec = gKernel[i];
        vec3 tangent = normalize(sampleVec - Normal * dot(sampleVec, Normal));
        vec3 bitangent = cross(Normal, tangent);
        vec3 sampleDir = tangent * gKernel[i].x + bitangent * gKernel[i].y + Normal * gKernel[i].z;
        
        vec3 samplePos = Pos + sampleDir * gSampleRad;
        vec4 offset = vec4(samplePos, 1.0);
        offset = gProj * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + vec2(0.5);

        float sampleDepth = texture(gPositionMap, offset.xy).z;
        
        // Range check and occlusion
        float rangeCheck = smoothstep(0.0, 1.0, gSampleRad / abs(Pos.z - sampleDepth));
        AO += (sampleDepth >= samplePos.z + 0.015 ? 1.0 : 0.0) * rangeCheck;
    }

    AO = 1.0 - AO / 64.0;

    FragColor = vec4(pow(AO, 1.5));
}