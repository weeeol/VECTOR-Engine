struct VSInput {
    uint vertexID : SV_VertexID;
};

struct VSOutput {
    float4 Pos : SV_POSITION;
    float2 TexCoords : TEXCOORD;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    // Generate fullscreen triangle
    float2 texCoords = float2((input.vertexID << 1) & 2, input.vertexID & 2);
    output.TexCoords = texCoords;
    output.Pos = float4(texCoords * 2.0f - 1.0f, 0.0f, 1.0f);
    output.Pos.y = -output.Pos.y; // DX12 Y is down
    return output;
}

struct PostProcessData {
    float exposure;
    int hdrTexIndex;
    int bloomTexIndex;
    int useBloom;
    float bloomIntensity;
    int padding[3];
};

ConstantBuffer<PostProcessData> ppData : register(b0);

SamplerState linearSampler : register(s0);

// ACES Filmic Tone Mapping
float3 ACESFilm(float3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 PSMain(VSOutput input) : SV_TARGET {
    Texture2D hdrTexture = ResourceDescriptorHeap[ppData.hdrTexIndex];
    float3 hdrColor = hdrTexture.Sample(linearSampler, input.TexCoords).rgb;
    
    if (ppData.useBloom > 0) {
        Texture2D bloomTexture = ResourceDescriptorHeap[ppData.bloomTexIndex];
        float3 bloomColor = bloomTexture.Sample(linearSampler, input.TexCoords).rgb;
        hdrColor += bloomColor * ppData.bloomIntensity; // Additive blending with intensity
    }
    
    // Apply exposure
    hdrColor *= ppData.exposure;
    
    // Tone mapping (ACES filmic)
    float3 mapped = ACESFilm(hdrColor);
    
    // Gamma correction 
    mapped = pow(mapped, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));
    
    return float4(mapped, 1.0f);
}
