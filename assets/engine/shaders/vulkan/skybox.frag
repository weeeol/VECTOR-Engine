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
    
    float cosAngle = dot(dir, normalize(pfd.sunDir.xyz));
    float sunElevation = clamp(normalize(pfd.sunDir.xyz).y, 0.0, 1.0);
    float sunsetIntensity = pow(max(0.00001, 1.0 - sunElevation), 4.0);
    vec3 sunsetColor = vec3(1.0, 0.5, 0.2);
    
    // Mie Scattering (Atmospheric Haze around the sun)
    float g = 0.985; 
    float g2 = g * g;
    float miePhase = (1.0 - g2) / (4.0 * 3.14159 * pow(max(0.0001, 1.0 + g2 - 2.0 * g * cosAngle), 1.5));
    
    // Tint the haze based on time of day
    vec3 mieTint = mix(vec3(1.0, 1.0, 1.0), sunsetColor, sunsetIntensity);
    color += mieTint * miePhase * 0.02;
    
    // Extra broad glow for the hazy look
    float broadGlow = pow(max(0.00001, cosAngle), 16.0) * 0.5;
    color += mieTint * broadGlow;
    
    // Smooth Sun core to avoid hard edges/falloff
    float sunCore = smoothstep(0.9996, 0.9999, cosAngle);
    color += vec3(20.0, 20.0, 20.0) * sunCore;

    FragColor = vec4(color, 1.0);
    
    mat4 rotView = mat4(mat3(pfd.view));
    mat4 prevRotView = mat4(mat3(pfd.prevView));
    
    mat4 unjitteredProj = pfd.projection;
    unjitteredProj[2][0] -= pfd.jitter.x;
    unjitteredProj[2][1] -= pfd.jitter.y;

    vec4 currentClip = unjitteredProj * rotView * vec4(dir, 0.0);
    vec4 prevClip = pfd.prevProjection * prevRotView * vec4(dir, 0.0);
    
    vec2 currentNDC = (currentClip.xy / currentClip.w) * 0.5 + 0.5;
    vec2 previousNDC = (prevClip.xy / prevClip.w) * 0.5 + 0.5;
    OutVelocity = currentNDC - previousNDC;
}
