#version 450
layout (location = 0) in vec3 FragPos;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoords;
layout (location = 3) in vec4 FragPosLightSpace;

layout (location = 0) out vec4 FragColor;

layout(set = 1, binding = 0) uniform sampler2D diffuseMap;

layout(push_constant) uniform MaterialData {
    layout(offset = 64) vec4 albedoColor;
    bool hasDiffuseMap;
} mat;

void main() {
    vec4 color = mat.albedoColor;
    if (mat.hasDiffuseMap) {
        color *= texture(diffuseMap, TexCoords);
    }
    FragColor = color;
}
