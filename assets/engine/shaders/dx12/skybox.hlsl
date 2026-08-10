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
    float3 color = float3(0.0f, 0.0f, 0.0f);
    
    float3 sunDir = normalize(pfd.sunDir.xyz);
    float sunElevation = clamp(sunDir.y, 0.0f, 1.0f);
    float sunsetIntensity = pow(1.0f - sunElevation, 4.0f);
    float3 sunsetColor = float3(1.0f, 0.5f, 0.2f); // Orange/Yellow
    
    if (material.hasAlbedoMap != 0 && material.albedoMapIndex >= 0) {
        TextureCube skyboxTex = ResourceDescriptorHeap[material.albedoMapIndex];
        color = skyboxTex.Sample(defaultSampler, dir).rgb;
    } else {
        // Procedural Atmospheric Sky Background
        float height = max(0.0f, dir.y);
        
        float3 zenithColor = float3(0.1f, 0.25f, 0.6f); // Deep blue
        float3 horizonColor = float3(0.6f, 0.7f, 0.85f); // Hazy light blue
        float3 groundColor = float3(0.15f, 0.15f, 0.15f);
        
        horizonColor = lerp(horizonColor, sunsetColor, sunsetIntensity);
        
        if (dir.y >= 0.0f) {
            color = lerp(horizonColor, zenithColor, pow(height, 0.6f));
        } else {
            color = lerp(horizonColor, groundColor, pow(abs(dir.y), 0.5f));
        }
    }
    
    // Add Sun and Atmospheric Haze over the sky background
    float cosAngle = dot(dir, sunDir);
    
    // Mie Scattering (Atmospheric Haze around the sun)
    float g = 0.985f; 
    float g2 = g * g;
    float miePhase = (1.0f - g2) / (4.0f * 3.14159f * pow(max(0.0001f, 1.0f + g2 - 2.0f * g * cosAngle), 1.5f));
    
    // Tint the haze based on time of day
    float3 mieTint = lerp(float3(1.0f, 1.0f, 1.0f), sunsetColor, sunsetIntensity);
    color += mieTint * miePhase * 0.02f;
    
    // Extra broad glow for the hazy look
    float broadGlow = pow(max(0.0f, cosAngle), 16.0f) * 0.5f;
    color += mieTint * broadGlow;
    
    // Smooth Sun core to avoid hard edges/falloff
    float sunCore = smoothstep(0.9996f, 0.9999f, cosAngle);
    color += float3(20.0f, 20.0f, 20.0f) * sunCore;
    
    return float4(color, 1.0f);
}
