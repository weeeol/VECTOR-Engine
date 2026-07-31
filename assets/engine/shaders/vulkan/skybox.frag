#version 450
layout (location = 0) in vec3 TexCoords;
layout (location = 1) in vec4 CurrentClipPos;
layout (location = 2) in vec4 PreviousClipPos;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec2 OutVelocity;

layout(set = 0, binding = 0) uniform PerFrameData {
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMatrix;
    vec4 viewPos;
    vec4 sunDir;
    vec4 sunColor;
    vec4 lightPos;
    vec4 lightColor;
    int ssaoEnabled;
    int padding;
    vec2 jitter;
    mat4 prevView;
    mat4 prevProjection;
} pfd;

layout(set = 1, binding = 0) uniform samplerCube skybox;

void main() {
    vec3 dir = normalize(TexCoords);
    vec3 color = texture(skybox, dir).rgb;
    
    // Procedural Sun
    float sunHit = dot(dir, normalize(pfd.sunDir.xyz));
    if (sunHit > 0.9998) {
        color = vec3(1.0, 1.0, 1.0) * 10.0;
    } else if (sunHit > 0.9980) {
        float glow = (sunHit - 0.9980) / (0.9998 - 0.9980);
        glow = pow(glow, 2.0);
        color += pfd.sunColor.rgb * glow * 3.0 + vec3(0.5, 0.5, 0.5) * glow;
    }

    FragColor = vec4(color, 1.0);
    
    vec2 currentNDC = (CurrentClipPos.xy / CurrentClipPos.w) * 0.5 + 0.5;
    vec2 previousNDC = (PreviousClipPos.xy / PreviousClipPos.w) * 0.5 + 0.5;
    OutVelocity = currentNDC - previousNDC;
}
