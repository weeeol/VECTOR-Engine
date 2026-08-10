struct BloomDataBlock {
    float2 srcResolution;
    float threshold;
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
    
    float2 texelSize = 1.0f / bloomData.srcResolution;
    float x = texelSize.x;
    float y = texelSize.y;

    float3 a = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x - 2*x, input.texcoord.y + 2*y), 0).rgb;
    float3 b = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x,       input.texcoord.y + 2*y), 0).rgb;
    float3 c = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x + 2*x, input.texcoord.y + 2*y), 0).rgb;

    float3 d = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x - 2*x, input.texcoord.y), 0).rgb;
    float3 e = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x,       input.texcoord.y), 0).rgb;
    float3 f = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x + 2*x, input.texcoord.y), 0).rgb;

    float3 g = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x - 2*x, input.texcoord.y - 2*y), 0).rgb;
    float3 h = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x,       input.texcoord.y - 2*y), 0).rgb;
    float3 i = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x + 2*x, input.texcoord.y - 2*y), 0).rgb;

    float3 j = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x - x, input.texcoord.y + y), 0).rgb;
    float3 k = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x + x, input.texcoord.y + y), 0).rgb;
    float3 l = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x - x, input.texcoord.y - y), 0).rgb;
    float3 m = srcTexture.SampleLevel(sampLinearClamp, float2(input.texcoord.x + x, input.texcoord.y - y), 0).rgb;

    float3 downsample = e * 0.125f;
    downsample += (a + c + g + i) * 0.03125f;
    downsample += (b + d + f + h) * 0.0625f;
    downsample += (j + k + l + m) * 0.125f;

    if (bloomData.threshold > 0.0f) {
        float brightness = max(downsample.r, max(downsample.g, downsample.b));
        float knee = bloomData.threshold * 0.5f;
        float soft = brightness - bloomData.threshold + knee;
        soft = clamp(soft, 0.0f, 2.0f * knee);
        soft = soft * soft / (4.0f * knee + 0.00001f);
        
        float contribution = max(soft, brightness - bloomData.threshold);
        downsample *= (contribution / max(brightness, 0.00001f));
    }

    return float4(downsample, 1.0f);
}
