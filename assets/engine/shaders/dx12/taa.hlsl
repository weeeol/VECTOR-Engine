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
    
    // Sample motion vector
    Texture2D<float2> motionVectorTex = ResourceDescriptorHeap[taaData.motionVectorTexIndex];
    float2 motionVector = motionVectorTex.SampleLevel(PointSampler, uv, 0);
    
    // Sample current frame
    Texture2D<float4> currentFrameTex = ResourceDescriptorHeap[taaData.currentFrameTexIndex];
    float3 currentColor = currentFrameTex.SampleLevel(LinearSampler, uv, 0).rgb;
    
    // Compute history UV
    float2 historyUV = uv - motionVector;
    
    // Out of bounds check
    if (historyUV.x < 0.0 || historyUV.x > 1.0 || historyUV.y < 0.0 || historyUV.y > 1.0) {
        return float4(currentColor, 1.0f);
    }
    
    // Sample history frame
    Texture2D<float4> historyTex = ResourceDescriptorHeap[taaData.historyTexIndex];
    float3 historyColor = historyTex.SampleLevel(LinearSampler, historyUV, 0).rgb;
    
    // Neighborhood clamping (to avoid ghosting)
    float2 texelSize = 1.0f / taaData.screenSize;
    float3 cMin = currentColor;
    float3 cMax = currentColor;
    
    // Sample a 3x3 neighborhood around current pixel
    [unroll]
    for (int x = -1; x <= 1; ++x) {
        [unroll]
        for (int y = -1; y <= 1; ++y) {
            if (x == 0 && y == 0) continue;
            float2 offsetUV = uv + float2(x, y) * texelSize;
            float3 neighbor = currentFrameTex.SampleLevel(PointSampler, offsetUV, 0).rgb;
            
            cMin = min(cMin, neighbor);
            cMax = max(cMax, neighbor);
        }
    }
    
    // Convert to YCoCg for clipping
    float3 historyYCoCg = RGBToYCoCg(historyColor);
    float3 cMinYCoCg = RGBToYCoCg(cMin);
    float3 cMaxYCoCg = RGBToYCoCg(cMax);
    
    // Clip history
    historyYCoCg = clamp(historyYCoCg, cMinYCoCg, cMaxYCoCg);
    historyColor = YCoCgToRGB(historyYCoCg);
    
    // Compute blend factor based on motion (less motion = trust history more)
    float velocityLength = length(motionVector * taaData.screenSize);
    float blendFactor = lerp(0.95f, 0.8f, saturate(velocityLength * 0.1f));
    
    // Blend current and history
    float3 resolvedColor = lerp(currentColor, historyColor, blendFactor);
    
    return float4(resolvedColor, 1.0f);
}
