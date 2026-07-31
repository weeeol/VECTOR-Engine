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
    float specularStrength;
    float shininess;
    int isUnlit;
    int hasTexture;
    int diffuseTextureIndex;
    int padding[3];
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

float3 CalcDirectionalLight(DirectionalLightData light, float3 normal, float3 viewDir, float matSpecStr, float matShininess, float4 fragPosLightSpace) {
    float3 lightDir = normalize(-light.direction.xyz);
    float3 lightColor = light.colorAndIntensity.xyz * light.colorAndIntensity.w;
    
    float diff = max(dot(normal, lightDir), 0.0f);
    
    float3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), matShininess);
    
    float3 diffuse = diff * lightColor;
    float3 specular = matSpecStr * spec * lightColor;
    
    float shadow = ShadowCalculation(fragPosLightSpace, normal, lightDir);
    
    return (1.0f - shadow) * (diffuse + specular);
}

float3 CalcPointLight(PointLightData light, float3 normal, float3 viewDir, float matSpecStr, float matShininess, float3 fragPos) {
    float3 lightDir = normalize(light.positionAndRadius.xyz - fragPos);
    float3 lightColor = light.colorAndIntensity.xyz * light.colorAndIntensity.w;
    float radius = light.positionAndRadius.w;
    
    float diff = max(dot(normal, lightDir), 0.0f);
    
    float3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), matShininess);
    
    float distance = length(light.positionAndRadius.xyz - fragPos);
    if(distance > radius) return float3(0.0f, 0.0f, 0.0f);
    
    float attenuation = pow(max(1.0f - (distance / radius), 0.0f), 2.0f);
    
    float3 diffuse = diff * lightColor;
    float3 specular = matSpecStr * spec * lightColor;
    
    return (diffuse + specular) * attenuation;
}

float4 PSMain(VSOutput input) : SV_TARGET {
    float3 baseObjectColor = material.albedoColor.rgb;
    float matSpecStr = material.specularStrength;
    float matShininess = material.shininess;
    bool matIsUnlit = material.isUnlit != 0;
    bool matHasTexture = material.hasTexture != 0;
    float3 vPos = pfd.viewPos.xyz;

    float3 baseColor = baseObjectColor;
    if (matHasTexture && material.diffuseTextureIndex >= 0) {
        Texture2D diffuseTex = ResourceDescriptorHeap[material.diffuseTextureIndex];
        baseColor = diffuseTex.Sample(defaultSampler, input.TexCoords).rgb * baseObjectColor;
    }

    if (matIsUnlit) {
        return float4(baseColor, 1.0f);
    }

    float3 norm = normalize(input.Normal);
    float3 viewDir = normalize(vPos - input.FragPos);

    float3 totalLighting = float3(0.0f, 0.0f, 0.0f);
    
    float ambientFactor = 1.0f;
    if (pfd.ssaoTexIndex >= 0) {
        Texture2D ssaoTex = ResourceDescriptorHeap[pfd.ssaoTexIndex];
        ambientFactor = ssaoTex.Load(int3(input.Pos.xy, 0)).r;
    }
    
    totalLighting += 0.3f * lightData.dirLight.colorAndIntensity.xyz * ambientFactor;

    totalLighting += CalcDirectionalLight(lightData.dirLight, norm, viewDir, matSpecStr, matShininess, input.FragPosLightSpace);

    for (int i = 0; i < lightData.numPointLights; i++) {
        totalLighting += CalcPointLight(lightData.pointLights[i], norm, viewDir, matSpecStr, matShininess, input.FragPos);
    }

    float3 result = totalLighting * baseColor;
    return float4(result, 1.0f);
}
