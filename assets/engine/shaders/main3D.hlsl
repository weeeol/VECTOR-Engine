cbuffer Constants : register(b0) {
    matrix model;
    matrix view;
    matrix projection;
    matrix lightSpaceMatrix;
    float3 objectColor;
    float padding1;
    float3 lightPos;
    float padding2;
    float3 lightColor;
    float padding3;
    float3 viewPos;
    float padding4;
    float3 sunDir;
    float padding5;
    int isUnlit;
    int hasTexture;
    int hasShadowMap; // unused padding
    int padding6;
};

Texture2D diffuseTexture : register(t0);
SamplerState smp : register(s0);

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 fragPos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    float4 worldPos = mul(model, float4(input.position, 1.0f));
    output.fragPos = worldPos.xyz;
    output.position = mul(projection, mul(view, worldPos));
    output.normal = mul((float3x3)model, input.normal); // Assume uniform scaling for simplicity
    output.texCoord = input.texCoord;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float4 color = float4(objectColor, 1.0f);
    if (hasTexture != 0) {
        color *= diffuseTexture.Sample(smp, input.texCoord);
    }
    
    if (isUnlit != 0) {
        return color;
    }
    
    // Ambient
    float ambientStrength = 0.3f;
    float3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    float3 norm = normalize(input.normal);
    float3 sun = normalize(sunDir);
    float diff = max(dot(norm, sun), 0.0f);
    float3 diffuse = diff * lightColor;
    
    float3 result = (ambient + diffuse) * color.rgb;
    return float4(result, color.a);
}
