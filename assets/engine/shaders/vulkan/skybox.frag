#version 450
layout (location = 0) in vec3 TexCoords;
layout (location = 1) in vec4 CurrentClipPos;
layout (location = 2) in vec4 PreviousClipPos;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec2 OutVelocity;

layout(set = 1, binding = 0) uniform samplerCube skybox;

void main() {
    vec3 color = texture(skybox, TexCoords).rgb;
    // Basic tone mapping (if not done in post process, but since we have bloom and post-process pass we will output HDR colors)
    FragColor = vec4(color, 1.0);
    
    vec2 currentNDC = (CurrentClipPos.xy / CurrentClipPos.w) * 0.5 + 0.5;
    vec2 previousNDC = (PreviousClipPos.xy / PreviousClipPos.w) * 0.5 + 0.5;
    OutVelocity = currentNDC - previousNDC;
}
