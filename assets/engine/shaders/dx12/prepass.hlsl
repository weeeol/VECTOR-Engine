#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

struct PerFrameData {
    matrix view;
    matrix projection;
    matrix previousView;
    matrix previousProjection;
    matrix lightSpaceMatrix;
    float4 viewPos;
    float4 sunDir;
    float4 sunColor;
    float4 lightPos;
    float4 lightColor;
    int shadowMapIndex;
    int ssaoTexIndex;
    float2 jitter;
    float2 previousJitter;
};

struct PerObjectData {
    matrix model;
    matrix finalBonesMatrices[MAX_BONES];
};

ConstantBuffer<PerFrameData> pfd : register(b0);
ConstantBuffer<PerObjectData> objectData : register(b2);

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
    int4 boneIds : BLENDINDICES;
    float4 weights : BLENDWEIGHT;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 viewNormal : NORMAL;
    float3 viewPos : POSITION1;
    float4 currentClipPos : TEXCOORD0;
    float4 previousClipPos : TEXCOORD1;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    
    float4 totalPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float3 totalNormal = float3(0.0f, 0.0f, 0.0f);
    bool hasBones = false;

    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        if (input.boneIds[i] == -1) continue;
        if (input.boneIds[i] >= MAX_BONES) {
            totalPosition = float4(input.position, 1.0f);
            break;
        }
        hasBones = true;
        
        matrix boneMatrix = objectData.finalBonesMatrices[input.boneIds[i]];
        float4 localPosition = mul(boneMatrix, float4(input.position, 1.0f));
        totalPosition += localPosition * input.weights[i];
        
        float3x3 boneRot = (float3x3)boneMatrix;
        float3 localNormal = mul(boneRot, input.normal);
        totalNormal += localNormal * input.weights[i];
    }

    if (!hasBones) {
        totalPosition = float4(input.position, 1.0f);
        totalNormal = input.normal;
    }

    float4 worldPos = mul(objectData.model, totalPosition);
    float4 viewPos = mul(pfd.view, worldPos);
    
    // Apply jitter to current frame projection
    output.position = mul(pfd.projection, viewPos);
    output.currentClipPos = output.position;
    
    // Compute previous clip pos WITHOUT current jitter, but with previous jitter
    float4 prevViewPos = mul(pfd.previousView, worldPos);
    output.previousClipPos = mul(pfd.previousProjection, prevViewPos);
    
    matrix modelView = mul(pfd.view, objectData.model);
    float3x3 normalMatrix = (float3x3)modelView;
    output.viewNormal = normalize(mul(normalMatrix, totalNormal));
    
    output.viewPos = viewPos.xyz;
    
    return output;
}

struct PSOutput {
    float4 normal : SV_Target0;
    float2 motionVector : SV_Target1;
};

PSOutput PSMain(VSOutput input) {
    PSOutput output;
    output.normal = float4(normalize(input.viewNormal), 1.0f);
    
    // Compute motion vector
    float2 currentNdc = input.currentClipPos.xy / input.currentClipPos.w;
    float2 previousNdc = input.previousClipPos.xy / input.previousClipPos.w;
    
    // Convert to UV space [0, 1]
    float2 currentUv = currentNdc * float2(0.5f, -0.5f) + 0.5f;
    float2 previousUv = previousNdc * float2(0.5f, -0.5f) + 0.5f;
    
    // Remove jitter from motion vector to prevent blurring static objects.
    // DX12 NDC Y is up, UV Y is down, so the jitter applied to NDC Y is inverted in UV Y.
    currentUv.x -= pfd.jitter.x;
    currentUv.y += pfd.jitter.y;
    previousUv.x -= pfd.previousJitter.x;
    previousUv.y += pfd.previousJitter.y;
    
    output.motionVector = currentUv - previousUv;
    
    return output;
}
