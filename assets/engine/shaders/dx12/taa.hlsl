struct TAADataBlock {
    float2 screenSize;
    int currentFrameTexIndex;
    int historyTexIndex;
    int motionVectorTexIndex;
    int depthTexIndex;
    int padding[2];
};

ConstantBuffer<TAADataBlock> taaData : register(b1);

SamplerState LinearSampler : register(s0);
SamplerState PointSampler : register(s1);

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

VSOutput VSMain(uint vertexID : SV_VertexID) {
    VSOutput output;
    output.texcoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}

float3 RGBToYCoCg(float3 rgb) {
    float y  =  0.25 * rgb.r + 0.5 * rgb.g + 0.25 * rgb.b;
    float co =  0.5  * rgb.r               - 0.5  * rgb.b;
    float cg = -0.25 * rgb.r + 0.5 * rgb.g - 0.25 * rgb.b;
    return float3(y, co, cg);
}

float3 YCoCgToRGB(float3 ycocg) {
    float r = ycocg.x + ycocg.y - ycocg.z;
    float g = ycocg.x           + ycocg.z;
    float b = ycocg.x - ycocg.y - ycocg.z;
    return float3(r, g, b);
}

float4 PSMain(VSOutput input) : SV_Target {
    float2 uv = input.texcoord;
    
    Texture2D<float2> motionVectorTex = ResourceDescriptorHeap[taaData.motionVectorTexIndex];
    Texture2D<float4> currentFrameTex = ResourceDescriptorHeap[taaData.currentFrameTexIndex];
    Texture2D<float4> historyTex = ResourceDescriptorHeap[taaData.historyTexIndex];
    
    float3 currentColor = currentFrameTex.SampleLevel(LinearSampler, uv, 0).rgb;
    
    float2 texelSize = 1.0f / taaData.screenSize;
    float2 velocity = motionVectorTex.SampleLevel(PointSampler, uv, 0);
    float maxVelocitySq = dot(velocity, velocity);
    
    [unroll]
    for (int y = -1; y <= 1; ++y) {
        [unroll]
        for (int x = -1; x <= 1; ++x) {
            if (x == 0 && y == 0) continue;
            float2 v = motionVectorTex.SampleLevel(PointSampler, uv + float2(x, y) * texelSize, 0);
            float vSq = dot(v, v);
            if (vSq > maxVelocitySq) {
                maxVelocitySq = vSq;
                velocity = v;
            }
        }
    }
    
    float2 historyUV = uv - velocity;
    
    if (historyUV.x < 0.0 || historyUV.x > 1.0 || historyUV.y < 0.0 || historyUV.y > 1.0) {
        return float4(currentColor, 1.0f);
    }
    
    float3 historyColor = historyTex.SampleLevel(LinearSampler, historyUV, 0).rgb;
    
    float3 currentYCoCg = RGBToYCoCg(currentColor);
    float3 historyYCoCg = RGBToYCoCg(historyColor);
    
    float3 minColor = currentYCoCg;
    float3 maxColor = currentYCoCg;
    
    [unroll]
    for (int y2 = -1; y2 <= 1; ++y2) {
        [unroll]
        for (int x2 = -1; x2 <= 1; ++x2) {
            if (x2 == 0 && y2 == 0) continue;
            float3 neighbor = RGBToYCoCg(currentFrameTex.SampleLevel(PointSampler, uv + float2(x2, y2) * texelSize, 0).rgb);
            minColor = min(minColor, neighbor);
            maxColor = max(maxColor, neighbor);
        }
    }
    
    float3 p_clip = 0.5f * (maxColor + minColor);
    float3 e_clip = 0.5f * (maxColor - minColor) + 0.0001f;
    float3 v_clip = historyYCoCg - p_clip;
    float3 v_unit = v_clip / e_clip;
    float3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));
    
    if (ma_unit > 1.0f) {
        historyYCoCg = p_clip + v_clip / ma_unit;
    }
    
    historyColor = YCoCgToRGB(historyYCoCg);
    
    float velocityLength = length(velocity);
    float baseBlend = 0.1f;
    float blend = lerp(baseBlend, 1.0f, clamp(velocityLength * 100.0f, 0.0f, 1.0f));
    
    float3 finalColor = lerp(historyColor, currentColor, blend);
    
    return float4(finalColor, 1.0f);
}
