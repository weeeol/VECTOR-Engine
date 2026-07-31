#version 450

layout (location = 0) out vec3 TexCoords;
layout (location = 1) out vec4 CurrentClipPos;
layout (location = 2) out vec4 PreviousClipPos;

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

const vec3 skyboxVertices[36] = vec3[36](
    vec3(-1.0,  1.0, -1.0), vec3(-1.0, -1.0, -1.0), vec3( 1.0, -1.0, -1.0),
    vec3( 1.0, -1.0, -1.0), vec3( 1.0,  1.0, -1.0), vec3(-1.0,  1.0, -1.0),
    vec3(-1.0, -1.0,  1.0), vec3(-1.0, -1.0, -1.0), vec3(-1.0,  1.0, -1.0),
    vec3(-1.0,  1.0, -1.0), vec3(-1.0,  1.0,  1.0), vec3(-1.0, -1.0,  1.0),
    vec3( 1.0, -1.0, -1.0), vec3( 1.0, -1.0,  1.0), vec3( 1.0,  1.0,  1.0),
    vec3( 1.0,  1.0,  1.0), vec3( 1.0,  1.0, -1.0), vec3( 1.0, -1.0, -1.0),
    vec3(-1.0, -1.0,  1.0), vec3(-1.0,  1.0,  1.0), vec3( 1.0,  1.0,  1.0),
    vec3( 1.0,  1.0,  1.0), vec3( 1.0, -1.0,  1.0), vec3(-1.0, -1.0,  1.0),
    vec3(-1.0,  1.0, -1.0), vec3( 1.0,  1.0, -1.0), vec3( 1.0,  1.0,  1.0),
    vec3( 1.0,  1.0,  1.0), vec3(-1.0,  1.0,  1.0), vec3(-1.0,  1.0, -1.0),
    vec3(-1.0, -1.0, -1.0), vec3(-1.0, -1.0,  1.0), vec3( 1.0, -1.0, -1.0),
    vec3( 1.0, -1.0, -1.0), vec3(-1.0, -1.0,  1.0), vec3( 1.0, -1.0,  1.0)
);

void main() {
    vec3 aPos = skyboxVertices[gl_VertexIndex];
    TexCoords = aPos;
    
    mat4 rotView = mat4(mat3(pfd.view));
    mat4 prevRotView = mat4(mat3(pfd.prevView));
    
    // Remove jitter from projection matrix for the current position used for velocity calculation
    mat4 unjitteredProj = pfd.projection;
    unjitteredProj[2][0] -= pfd.jitter.x;
    unjitteredProj[2][1] -= pfd.jitter.y;

    CurrentClipPos = unjitteredProj * rotView * vec4(aPos, 1.0);
    PreviousClipPos = pfd.prevProjection * prevRotView * vec4(aPos, 1.0);
    
    vec4 pos = pfd.projection * rotView * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
