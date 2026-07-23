#version 450
layout (location = 0) in vec3 TexCoords;

layout (location = 0) out vec4 FragColor;

layout(set = 1, binding = 0) uniform samplerCube skybox;

void main() {
    vec3 color = texture(skybox, TexCoords).rgb;
    // Basic tone mapping (if not done in post process, but since we have bloom and post-process pass we will output HDR colors)
    FragColor = vec4(color, 1.0);
}
