#version 450

layout (location = 0) in vec2 TexCoords;
layout (location = 0) out vec4 FragColor;

layout (set = 0, binding = 0) uniform sampler2D screenTexture;
layout (set = 0, binding = 1) uniform sampler2D bloomTexture;

layout (push_constant) uniform PushConstants {
    float exposure;
    float bloomStrength;
} pc;

// ACES Filmic Tone Mapping
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    // Sample HDR scene color
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;

    // Add bloom
    vec3 bloom = texture(bloomTexture, TexCoords).rgb;
    hdrColor += bloom * pc.bloomStrength;

    // Apply exposure
    hdrColor *= pc.exposure;

    // Tone mapping (ACES filmic)
    vec3 mapped = ACESFilm(hdrColor);

    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / 2.2));

    FragColor = vec4(mapped, 1.0);
}
