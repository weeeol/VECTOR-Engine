struct BloomDataBlock {
    float filterRadius;
    int srcTexIndex;
};

ConstantBuffer<BloomDataBlock> bloomData : register(b0);
SamplerState sampLinearClamp : register(s0);

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 PSMain(VSOutput input) : SV_Target {
    Texture2D srcTexture = ResourceDescriptorHeap[bloomData.srcTexIndex];
    
    float x = bloomData.filterRadius;
    float y = bloomData.filterRadius;

    float3 a = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x - x, input.texcoord.y + y), 0).rgb;
    float3 b = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x,     input.texcoord.y + y), 0).rgb;
    float3 c = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x + x, input.texcoord.y + y), 0).rgb;

    float3 d = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x - x, input.texcoord.y), 0).rgb;
    float3 e = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x,     input.texcoord.y), 0).rgb;
    float3 f = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x + x, input.texcoord.y), 0).rgb;

    float3 g = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x - x, input.texcoord.y - y), 0).rgb;
    float3 h = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x,     input.texcoord.y - y), 0).rgb;
    float3 i = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x + x, input.texcoord.y - y), 0).rgb;

    float3 upsample = e * 4.0f;
    upsample += (b + d + f + h) * 2.0f;
    upsample += (a + c + g + i);
    upsample *= 1.0f / 16.0f;

    return float4(upsample, 1.0f);
}
