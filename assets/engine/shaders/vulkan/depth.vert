#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 albedoColor;
    uint hasDiffuseMap;
} pc;

layout(set = 0, binding = 0) uniform PerFrameData {
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMatrix;
    vec3 viewPos;
} ubo;

void main() {
    gl_Position = ubo.lightSpaceMatrix * pc.model * vec4(inPosition, 1.0);
}
