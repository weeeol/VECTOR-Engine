#version 450
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 boneIds;
layout (location = 4) in vec4 weights;

layout (location = 0) out vec3 FragPos;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec2 TexCoords;
layout (location = 3) out vec4 FragPosLightSpace;
layout (location = 4) out vec4 CurrentClipPos;
layout (location = 5) out vec4 PreviousClipPos;

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

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

layout(set = 2, binding = 0) uniform BoneData {
    mat4 finalBonesMatrices[MAX_BONES];
} boneData;

void main() {
    vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f);
    bool hasBones = false;

    for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
    {
        if(boneIds[i] == -1) 
            continue;
        if(boneIds[i] >= MAX_BONES) 
        {
            totalPosition = vec4(aPos,1.0f);
            break;
        }
        hasBones = true;
        vec4 localPosition = boneData.finalBonesMatrices[boneIds[i]] * vec4(aPos,1.0f);
        totalPosition += localPosition * weights[i];
        vec3 localNormal = mat3(boneData.finalBonesMatrices[boneIds[i]]) * aNormal;
        totalNormal += localNormal * weights[i];
    }

    if (!hasBones) {
        totalPosition = vec4(aPos, 1.0f);
        totalNormal = aNormal;
    }

    FragPos = vec3(pc.model * totalPosition);
    Normal = mat3(transpose(inverse(pc.model))) * totalNormal;
    TexCoords = aTexCoords;
    FragPosLightSpace = pfd.lightSpaceMatrix * vec4(FragPos, 1.0);

    // Remove jitter from projection matrix for the current position used for velocity calculation
    mat4 unjitteredProj = pfd.projection;
    unjitteredProj[2][0] -= pfd.jitter.x;
    unjitteredProj[2][1] -= pfd.jitter.y;

    CurrentClipPos = unjitteredProj * pfd.view * vec4(FragPos, 1.0);
    // Since we don't have prevModel, we assume static for now or use current model. 
    // For skinned meshes we should ideally use previous bone matrices, but we'll use current for simplicity unless added later.
    PreviousClipPos = pfd.prevProjection * pfd.prevView * vec4(FragPos, 1.0);
    
    gl_Position = pfd.projection * pfd.view * vec4(FragPos, 1.0);
}
