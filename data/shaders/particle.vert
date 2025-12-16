#version 330 core

// Input vertex attributes
layout(location = 0) in vec3 aPosition;     // Vertex position
layout(location = 1) in vec2 aTexCoord;     // Texture coordinates

// Instanced attributes
layout(location = 2) in vec3 iPosition;     // Particle position
layout(location = 3) in float iSize;        // Particle size
layout(location = 4) in vec4 iColor;        // Particle color
layout(location = 5) in float iRotation;    // Particle rotation
layout(location = 6) in float iLife;        // Particle life

// Uniform variables
uniform mat4 model;              // Model matrix
uniform mat4 view;               // View matrix
uniform mat4 projection;         // Projection matrix
uniform vec3 viewPosition;       // Viewer position

// Output to fragment shader
out vec2 TexCoord;               // Texture coordinates
out vec4 ParticleColor;          // Particle color
out float ParticleLife;          // Particle life
out vec3 WorldPos;               // World position
out vec3 ViewPos;                // View space position
out vec3 Normal;                 // Normal (for lighting calculation)

void main() {
    // Calculate rotation matrix
    float cosR = cos(iRotation);
    float sinR = sin(iRotation);
    mat2 rotationMatrix = mat2(cosR, -sinR, sinR, cosR);
    
    // Apply rotation to vertex position
    vec2 rotatedPosition = rotationMatrix * aPosition.xy * iSize;
    
    // Calculate world position
    vec3 worldPos = iPosition + vec3(rotatedPosition, aPosition.z);
    
    // Calculate final position
    gl_Position = projection * view * model * vec4(worldPos, 1.0);
    
    // Pass texture coordinates
    TexCoord = aTexCoord;
    
    // Pass particle color and life
    ParticleColor = iColor;
    ParticleLife = iLife;
    
    // Calculate world and view space positions
    WorldPos = worldPos;
    ViewPos = vec3(view * model * vec4(worldPos, 1.0));
    
    // Calculate normal (facing viewer)
    Normal = normalize(viewPosition - worldPos);
}