struct PerFrameData {
    matrix view;
    matrix projection;
    matrix lightSpaceMatrix;
    float4 viewPos;
    float4 sunDir;
    float4 sunColor;
    float4 lightPos;
    float4 lightColor;
};

ConstantBuffer<PerFrameData> pfd : register(b0);

struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 TexCoords : TEXCOORD;
};

static const float3 skyboxVertices[36] = {
    float3(-1.0,  1.0, -1.0), float3(-1.0, -1.0, -1.0), float3( 1.0, -1.0, -1.0),
    float3( 1.0, -1.0, -1.0), float3( 1.0,  1.0, -1.0), float3(-1.0,  1.0, -1.0),
    float3(-1.0, -1.0,  1.0), float3(-1.0, -1.0, -1.0), float3(-1.0,  1.0, -1.0),
    float3(-1.0,  1.0, -1.0), float3(-1.0,  1.0,  1.0), float3(-1.0, -1.0,  1.0),
    float3( 1.0, -1.0, -1.0), float3( 1.0, -1.0,  1.0), float3( 1.0,  1.0,  1.0),
    float3( 1.0,  1.0,  1.0), float3( 1.0,  1.0, -1.0), float3( 1.0, -1.0, -1.0),
    float3(-1.0, -1.0,  1.0), float3(-1.0,  1.0,  1.0), float3( 1.0,  1.0,  1.0),
    float3( 1.0,  1.0,  1.0), float3( 1.0, -1.0,  1.0), float3(-1.0, -1.0,  1.0),
    float3(-1.0,  1.0, -1.0), float3( 1.0,  1.0, -1.0), float3( 1.0,  1.0,  1.0),
    float3( 1.0,  1.0,  1.0), float3(-1.0,  1.0,  1.0), float3(-1.0,  1.0, -1.0),
    float3(-1.0, -1.0, -1.0), float3(-1.0, -1.0,  1.0), float3( 1.0, -1.0, -1.0),
    float3( 1.0, -1.0, -1.0), float3(-1.0, -1.0,  1.0), float3( 1.0, -1.0,  1.0)
};

VSOutput VSMain(uint vertexID : SV_VertexID) {
    VSOutput output;
    float3 aPos = skyboxVertices[vertexID];
    output.TexCoords = aPos;
    
    // Remove translation by casting to 3x3 then back to 4x4
    float3x3 rotView3x3 = (float3x3)pfd.view;
    float4x4 rotView = float4x4(
        float4(rotView3x3[0], 0.0f),
        float4(rotView3x3[1], 0.0f),
        float4(rotView3x3[2], 0.0f),
        float4(0.0f, 0.0f, 0.0f, 1.0f)
    );
    
    float4 pos = mul(pfd.projection, mul(rotView, float4(aPos, 1.0f)));
    
    // Set z = w so that depth is always 1.0 (furthest)
    output.Pos = pos.xyww;
    return output;
}

SamplerState defaultSampler : register(s0);

struct MaterialData {
    float4 albedoColor;
    int hasAlbedoMap;
    int hasNormalMap;
    int hasMetallicRoughnessMap;
    int hasAOMap;
    float metallicFactor;
    float roughnessFactor;
    int isUnlit;
    int padding;
    int albedoMapIndex;
    int normalMapIndex;
    int metallicRoughnessMapIndex;
    int aoMapIndex;
};

ConstantBuffer<MaterialData> material : register(b3);

float4 PSMain(VSOutput input) : SV_TARGET {
    float3 dir = normalize(input.TexCoords);
    float3 color = float3(0.1f, 0.1f, 0.12f);
    
    if (material.hasAlbedoMap != 0 && material.albedoMapIndex >= 0) {
        TextureCube skyboxTex = ResourceDescriptorHeap[material.albedoMapIndex];
        color = skyboxTex.Sample(defaultSampler, dir).rgb;
    }
    
    // Procedural Sun
    // sunDir points towards the sun (from origin)
    float sunHit = dot(dir, normalize(pfd.sunDir.xyz));
    if (sunHit > 0.99995) {
        // Sun core (very bright, pure white, smaller)
        color = float3(1.0, 1.0, 1.0) * 15.0;
    } else if (sunHit > 0.9995) {
        // Sun halo / glow (smooth falloff, slightly warm white, smaller)
        float glow = (sunHit - 0.9995) / (0.99995 - 0.9995);
        // Exponential falloff for a more natural glow
        glow = pow(glow, 3.0);
        color += float3(1.0, 0.9, 0.8) * pfd.sunColor.rgb * glow * 5.0 + float3(0.5, 0.5, 0.5) * glow;
    }
    
    return float4(color, 1.0);
}
