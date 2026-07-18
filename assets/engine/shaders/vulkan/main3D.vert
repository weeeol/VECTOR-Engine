#version 450
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

layout (location = 0) out vec3 FragPos;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec2 TexCoords;
layout (location = 3) out vec4 FragPosLightSpace;

layout(set = 0, binding = 0) uniform PerFrameData {
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMatrix;
    vec4 viewPos;
    vec4 sunDir;
    vec4 sunColor;
    vec4 lightPos;
    vec4 lightColor;
} pfd;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

void main() {
    FragPos = vec3(pc.model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(pc.model))) * aNormal;
    TexCoords = aTexCoords;
    FragPosLightSpace = pfd.lightSpaceMatrix * vec4(FragPos, 1.0);
    gl_Position = pfd.projection * pfd.view * vec4(FragPos, 1.0);
}
