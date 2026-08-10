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
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoords : TEXCOORD;
    int4 BoneIds : BLENDINDICES;
    float4 Weights : BLENDWEIGHT;
};

struct VSOutput {
    float4 Pos : SV_POSITION;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    
    float4 totalPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
    bool hasBones = false;

    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        if (input.BoneIds[i] == -1) continue;
        if (input.BoneIds[i] >= MAX_BONES) {
            totalPosition = float4(input.Pos, 1.0f);
            break;
        }
        hasBones = true;
        
        matrix boneMat = objectData.finalBonesMatrices[input.BoneIds[i]];
        float4 localPosition = mul(boneMat, float4(input.Pos, 1.0f));
        totalPosition += localPosition * input.Weights[i];
    }

    if (!hasBones) {
        totalPosition = float4(input.Pos, 1.0f);
    }

    output.Pos = mul(pfd.lightSpaceMatrix, mul(objectData.model, totalPosition));
    return output;
}

// We just need depth, so the pixel shader can be empty or omitted.
// DirectX 12 allows omitting the pixel shader if depth is the only output.
void PSMain() {
}
