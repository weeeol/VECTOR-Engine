struct SSAODataBlock {
    matrix projection;
    matrix invProjection;
    float4 samples[64];
    float radius;
    float bias;
    float2 screenSize;
    int normalTexIndex;
    int depthTexIndex;
    int noiseTexIndex;
    int ssaoTexIndex;
};

ConstantBuffer<SSAODataBlock> ssaoData : register(b0);

SamplerState sampPointClamp : register(s2);

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 PSMain(VSOutput input) : SV_Target {
    Texture2D texSSAO = ResourceDescriptorHeap[ssaoData.ssaoTexIndex];
    float2 texelSize = 1.0f / ssaoData.screenSize;
    float result = 0.0f;
    
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            float2 offset = float2(float(x), float(y)) * texelSize;
            result += texSSAO.SampleLevel(sampPointClamp, input.texcoord + offset, 0).r;
        }
    }
    
    float blur = result / (4.0f * 4.0f);
    return float4(blur, blur, blur, 1.0f);
}
