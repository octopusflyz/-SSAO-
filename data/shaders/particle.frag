#version 330 core

// Variables passed from vertex shader
in vec2 TexCoord;               // Texture coordinates
in vec4 ParticleColor;          // Particle color
in float ParticleLife;          // Particle life
in vec3 WorldPos;               // World position
in vec3 ViewPos;                // View space position
in vec3 Normal;                 // Normal

// Output color
out vec4 FragColor;

// Uniform variables
uniform sampler2D particleTexture;   // Particle texture
uniform bool enableLighting;          // Whether to enable lighting
uniform float particleAlpha;          // Particle alpha coefficient

// Lighting parameters
uniform int lightType;                // Light type (0=directional, 1=point)
uniform vec3 lightPosition;           // Light position
uniform vec3 lightDirection;          // Light direction
uniform vec3 lightColor;              // Light color
uniform float lightIntensity;         // Light intensity
uniform float ambientStrength;        // Ambient light strength

// Calculate lighting
vec3 calculateLighting(vec3 normal, vec3 viewDir, vec3 particleColor) {
    if (!enableLighting) {
        return particleColor;
    }
    
    vec3 lightDir;
    float attenuation = 1.0;
    
    if (lightType == 0) {
        // Directional light
        lightDir = normalize(-lightDirection);
    } else {
        // Point light
        vec3 lightVec = lightPosition - WorldPos;
        lightDir = normalize(lightVec);
        
        // Distance attenuation
        float distance = length(lightVec);
        attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    }
    
    // Ambient light
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse reflection
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular reflection (simple Phong model)
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spec * lightColor;
    
    // Combine lighting results
    vec3 result = (ambient + diffuse + specular) * particleColor * lightIntensity * attenuation;
    
    return result;
}

void main() {
    // Sample texture
    vec4 texColor = texture(particleTexture, TexCoord);
    
    // Discard if texture is transparent
    if (texColor.a < 0.01) {
        discard;
    }
    
    // Calculate final color
    vec3 finalColor;
    
    if (enableLighting) {
        // Calculate lighting
        vec3 viewDir = normalize(-ViewPos);
        finalColor = calculateLighting(Normal, viewDir, ParticleColor.rgb);
    } else {
        // Don't use lighting, directly use particle color
        finalColor = ParticleColor.rgb;
    }
    
    // Apply texture color
    finalColor *= texColor.rgb;
    
    // Calculate final alpha
    float finalAlpha = texColor.a * ParticleColor.a * ParticleLife * particleAlpha;
    
    // Output final color
    FragColor = vec4(finalColor, finalAlpha);
}