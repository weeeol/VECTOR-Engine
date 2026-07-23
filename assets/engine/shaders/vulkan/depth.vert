#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in ivec4 boneIds;
layout(location = 4) in vec4 weights;

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

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

layout(set = 2, binding = 0) uniform BoneData {
    mat4 finalBonesMatrices[MAX_BONES];
} boneData;

void main() {
    vec4 totalPosition = vec4(0.0f);
    bool hasBones = false;

    for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
    {
        if(boneIds[i] == -1) 
            continue;
        if(boneIds[i] >= MAX_BONES) 
        {
            totalPosition = vec4(inPosition,1.0f);
            break;
        }
        hasBones = true;
        vec4 localPosition = boneData.finalBonesMatrices[boneIds[i]] * vec4(inPosition,1.0f);
        totalPosition += localPosition * weights[i];
    }

    if (!hasBones) {
        totalPosition = vec4(inPosition, 1.0f);
    }

    gl_Position = ubo.lightSpaceMatrix * pc.model * totalPosition;
}
