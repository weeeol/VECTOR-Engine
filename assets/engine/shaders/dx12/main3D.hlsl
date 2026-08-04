#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4
#define MAX_POINT_LIGHTS 64

struct PerFrameData {
    matrix view;
    matrix projection;
    matrix lightSpaceMatrix;
    float4 viewPos;
    float4 sunDir;
    float4 sunColor;
    float4 lightPos;
    float4 lightColor;
    int shadowMapIndex;
    int ssaoTexIndex;
    int padding[2];
};

struct PointLightData {
    float4 positionAndRadius;
    float4 colorAndIntensity;
};

struct DirectionalLightData {
    float4 direction;
    float4 colorAndIntensity;
};

struct LightDataBlock {
    DirectionalLightData dirLight;
    PointLightData pointLights[MAX_POINT_LIGHTS];
    int numPointLights;
    float3 padding;
};

struct PerObjectData {
    matrix model;
    matrix finalBonesMatrices[MAX_BONES];
};

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

ConstantBuffer<PerFrameData> pfd : register(b0);
ConstantBuffer<LightDataBlock> lightData : register(b1);
ConstantBuffer<PerObjectData> objectData : register(b2);
ConstantBuffer<MaterialData> material : register(b3);

struct VSInput {
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoords : TEXCOORD;
    int4 BoneIds : BLENDINDICES;
    float4 Weights : BLENDWEIGHT;
};

struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 FragPos : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoords : TEXCOORD;
    float4 FragPosLightSpace : TEXCOORD1;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    
    float4 totalPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float3 totalNormal = float3(0.0f, 0.0f, 0.0f);
    bool hasBones = false;

    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        if (input.BoneIds[i] == -1) continue;
        if (input.BoneIds[i] >= MAX_BONES) {
            totalPosition = float4(input.Pos, 1.0f);
            totalNormal = input.Normal;
            break;
        }
        hasBones = true;
        
        matrix boneMat = objectData.finalBonesMatrices[input.BoneIds[i]];
        float4 localPosition = mul(boneMat, float4(input.Pos, 1.0f));
        totalPosition += localPosition * input.Weights[i];
        
        float3 localNormal = mul((float3x3)boneMat, input.Normal);
        totalNormal += localNormal * input.Weights[i];
    }

    if (!hasBones) {
        totalPosition = float4(input.Pos, 1.0f);
        totalNormal = input.Normal;
    }

    output.FragPos = mul(objectData.model, totalPosition).xyz;
    
    float3x3 normalMatrix = (float3x3)objectData.model;
    output.Normal = mul(normalMatrix, totalNormal);
    
    output.TexCoords = input.TexCoords;
    output.FragPosLightSpace = mul(pfd.lightSpaceMatrix, float4(output.FragPos, 1.0f));
    output.Pos = mul(pfd.projection, mul(pfd.view, float4(output.FragPos, 1.0f)));
    
    return output;
}

SamplerState defaultSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

float ShadowCalculation(float4 fragPosLightSpace, float3 normal, float3 lightDir) {
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f; // DX12 Y is down
    
    if (projCoords.z > 1.0f) return 0.0f;
    if (pfd.shadowMapIndex < 0) return 0.0f;

    Texture2D shadowMap = ResourceDescriptorHeap[pfd.shadowMapIndex];
    
    float currentDepth = projCoords.z;
    float bias = max(0.05f * (1.0f - dot(normal, lightDir)), 0.005f);
    
    float shadow = 0.0f;
    float2 texelSize = 1.0f / 2048.0f; 

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(shadowSampler, projCoords.xy + offset, currentDepth - bias);
        }
    }
    shadow /= 9.0f;
    return 1.0f - shadow;
}

static const float PI = 3.14159265359f;

float DistributionGGX(float3 N, float3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

float3 getNormalFromMap(float3 fragNormal, float2 texCoords, float3 fragPos) {
    Texture2D normalMapTex = ResourceDescriptorHeap[material.normalMapIndex];
    float3 tangentNormal = normalMapTex.Sample(defaultSampler, texCoords).xyz * 2.0f - 1.0f;

    float3 Q1  = ddx(fragPos);
    float3 Q2  = ddy(fragPos);
    float2 st1 = ddx(texCoords);
    float2 st2 = ddy(texCoords);

    float3 N   = normalize(fragNormal);
    float3 T   = normalize(Q1 * st2.y - Q2 * st1.y);
    float3 B   = -normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(tangentNormal, TBN)); // In HLSL, vector * matrix or mul(vec, mat). Actually it's TBN is column vectors.
    // wait! T, B, N are rows in TBN if float3x3(T, B, N). So we should mul(tangentNormal, TBN).
}

float4 PSMain(VSOutput input) : SV_TARGET {
    float4 albedoTex = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (material.hasAlbedoMap != 0 && material.albedoMapIndex >= 0) {
        Texture2D diffuseTex = ResourceDescriptorHeap[material.albedoMapIndex];
        albedoTex = diffuseTex.Sample(defaultSampler, input.TexCoords);
    }
    
    // gamma correction for albedo map
    float3 albedo = pow(albedoTex.rgb * material.albedoColor.rgb, float3(2.2f, 2.2f, 2.2f));
    float alpha = albedoTex.a * material.albedoColor.a;

    if (material.isUnlit != 0) {
        return float4(albedo, alpha);
    }

    float metallic = material.metallicFactor;
    float roughness = material.roughnessFactor;
    if (material.hasMetallicRoughnessMap != 0 && material.metallicRoughnessMapIndex >= 0) {
        Texture2D mrTex = ResourceDescriptorHeap[material.metallicRoughnessMapIndex];
        float4 mr = mrTex.Sample(defaultSampler, input.TexCoords);
        metallic *= mr.b;
        roughness *= mr.g;
    }

    float ao = 1.0f;
    if (material.hasAOMap != 0 && material.aoMapIndex >= 0) {
        Texture2D aoTex = ResourceDescriptorHeap[material.aoMapIndex];
        ao = aoTex.Sample(defaultSampler, input.TexCoords).r;
    }

    float3 N = normalize(input.Normal);
    if (material.hasNormalMap != 0 && material.normalMapIndex >= 0) {
        N = getNormalFromMap(input.Normal, input.TexCoords, input.FragPos);
    }
    
    float3 V = normalize(pfd.viewPos.xyz - input.FragPos);

    float3 F0 = float3(0.04f, 0.04f, 0.04f); 
    F0 = lerp(F0, albedo, metallic);

    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    // Directional light
    {
        float3 L = normalize(-lightData.dirLight.direction.xyz);
        float3 H = normalize(V + L);
        float3 radiance = lightData.dirLight.colorAndIntensity.xyz * lightData.dirLight.colorAndIntensity.w;

        float NDF = DistributionGGX(N, H, roughness);       
        float G   = GeometrySmith(N, V, L, roughness);      
        float3 F  = fresnelSchlick(max(dot(H, V), 0.0f), F0);       

        float3 kS = F;
        float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
        kD *= 1.0f - metallic;    

        float3 numerator    = NDF * G * F;
        float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f;
        float3 specular     = numerator / denominator;  

        float NdotL = max(dot(N, L), 0.0f);        
        
        float shadow = ShadowCalculation(input.FragPosLightSpace, N, L);
        Lo += (1.0f - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Point lights
    for (int i = 0; i < lightData.numPointLights; ++i) {
        float3 lightPos = lightData.pointLights[i].positionAndRadius.xyz;
        float3 L = normalize(lightPos - input.FragPos);
        float3 H = normalize(V + L);
        
        float distance = length(lightPos - input.FragPos);
        float attenuation = 1.0f / (distance * distance);
        float3 radiance = lightData.pointLights[i].colorAndIntensity.xyz * lightData.pointLights[i].colorAndIntensity.w * attenuation;
        
        float NDF = DistributionGGX(N, H, roughness);       
        float G   = GeometrySmith(N, V, L, roughness);      
        float3 F  = fresnelSchlick(max(dot(H, V), 0.0f), F0);       

        float3 kS = F;
        float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
        kD *= 1.0f - metallic;    

        float3 numerator    = NDF * G * F;
        float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f;
        float3 specular     = numerator / denominator;  

        float NdotL = max(dot(N, L), 0.0f);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    float ssao = 1.0f;
    if (pfd.ssaoTexIndex >= 0) {
        Texture2D ssaoTex = ResourceDescriptorHeap[pfd.ssaoTexIndex];
        ssao = ssaoTex.Load(int3(input.Pos.xy, 0)).r;
    }

    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * ao * ssao;
    
    float3 color = ambient + Lo;

    return float4(color, alpha);
}
