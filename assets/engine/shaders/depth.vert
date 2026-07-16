#version 330 core
layout (location = 0) in vec3 aPos;

// Per-frame data from UBO (binding point 0)
layout(std140) uniform PerFrameData {
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMatrix;
    vec4 viewPos;
    vec4 sunDir;
    vec4 sunColor;
    vec4 lightPos;
    vec4 lightColor;
} pfd;

// Per-object uniform
uniform mat4 model;

void main() {
    gl_Position = pfd.lightSpaceMatrix * model * vec4(aPos, 1.0);
}
