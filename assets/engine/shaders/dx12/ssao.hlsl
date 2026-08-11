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
    int padding;
};

ConstantBuffer<SSAODataBlock> ssaoData : register(b0);

SamplerState sampPointClamp : register(s2);
SamplerState sampPointWrap  : register(s3);

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float3 ReconstructViewPosition(float2 texCoords, float depth) {
    // DX12 NDC: X: [-1, 1], Y: [1, -1] (Y is up), Z: [0, 1]
    float4 ndc = float4(
        texCoords.x * 2.0f - 1.0f,
        -texCoords.y * 2.0f + 1.0f,
        depth,
        1.0f
    );
    
    float4 viewPos = mul(ssaoData.invProjection, ndc);
    return viewPos.xyz / viewPos.w;
}

float4 PSMain(VSOutput input) : SV_Target {
    Texture2D texDepth = ResourceDescriptorHeap[ssaoData.depthTexIndex];
    Texture2D texNormal = ResourceDescriptorHeap[ssaoData.normalTexIndex];
    Texture2D texNoise = ResourceDescriptorHeap[ssaoData.noiseTexIndex];

    float depth = texDepth.SampleLevel(sampPointClamp, input.texcoord, 0).r;
    
    // Don't calculate SSAO for skybox (depth = 1.0)
    if (depth >= 1.0f) {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    float3 fragPos = ReconstructViewPosition(input.texcoord, depth);
    float3 normal = normalize(texNormal.SampleLevel(sampPointClamp, input.texcoord, 0).rgb);
    
    // Tile noise texture
    float2 noiseScale = ssaoData.screenSize / 4.0f;
    float3 randomVec = normalize(texNoise.SampleLevel(sampPointWrap, input.texcoord * noiseScale, 0).xyz);
    
    // TBN Matrix
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal); 
    
    float occlusion = 0.0f;
    int kernelSize = 64;
    
    for (int i = 0; i < kernelSize; ++i) {
        // from tangent to view-space
        float3 samplePos = ssaoData.samples[i].x * tangent + ssaoData.samples[i].y * bitangent + ssaoData.samples[i].z * normal;
        samplePos = fragPos + samplePos * ssaoData.radius;
        
        // project sample position
        float4 offset = float4(samplePos, 1.0f);
        offset = mul(ssaoData.projection, offset); // view to clip
        offset.xyz /= offset.w; // perspective divide
        
        // clip to texcoord
        offset.x = offset.x * 0.5f + 0.5f;
        offset.y = -offset.y * 0.5f + 0.5f;
        
        float sampleDepth = texDepth.SampleLevel(sampPointClamp, offset.xy, 0).r;
        float3 sampleViewPos = ReconstructViewPosition(offset.xy, sampleDepth);
        
        float rangeCheck = smoothstep(0.0f, 1.0f, ssaoData.radius / abs(fragPos.z - sampleViewPos.z));
        occlusion += (sampleViewPos.z >= samplePos.z + ssaoData.bias ? 1.0f : 0.0f) * rangeCheck;
    }
    
    occlusion = 1.0f - (occlusion / (float)kernelSize);
    return float4(occlusion, occlusion, occlusion, 1.0f);
}


