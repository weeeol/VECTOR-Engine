#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gMetallicRoughnessAO;
uniform sampler2D shadowMap;

// Per-frame data from UBO (binding point 0)
layout(std140) uniform PerFrameData {
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMatrix;
    vec4 viewPos;
    vec4 sunDir;
    vec4 sunColor;
    vec4 lightPos;
    vec4 lightColor;
} pfd;

// Multi-Light data from UBO (binding point 1)
#define MAX_POINT_LIGHTS 64

struct PointLightData {
    vec4 positionAndRadius;
    vec4 colorAndIntensity;
};

struct DirectionalLightData {
    vec4 direction;
    vec4 colorAndIntensity;
};

layout(std140) uniform LightDataBlock {
    DirectionalLightData dirLight;
    PointLightData pointLights[MAX_POINT_LIGHTS];
    int numPointLights;
} lightData;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ----------------------------------------------------------------------------
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0)
        return 0.0;
        
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

// ----------------------------------------------------------------------------
vec3 ComputePBR(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float roughness, float metallic) {
    vec3 H = normalize(V + L);
    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);           
    
    vec3 nominator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
    vec3 specular = nominator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  
    
    float NdotL = max(dot(N, L), 0.0);        

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main() {             
    // Sample G-Buffer attributes
    vec4 albedoData = texture(gAlbedo, TexCoords);
    vec3 albedo = albedoData.rgb;
    float isUnlit = albedoData.a;
    
    // If the material is marked as unlit, output color immediately
    if (isUnlit > 0.5) {
        FragColor = vec4(albedo, 1.0);
        return;
    }
    
    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    vec3 normal = texture(gNormal, TexCoords).rgb;
    vec3 metallicRoughnessAO = texture(gMetallicRoughnessAO, TexCoords).rgb;
    
    float metallic = metallicRoughnessAO.r;
    float roughness = metallicRoughnessAO.g;
    float ao = metallicRoughnessAO.b;

    vec3 N = normalize(normal);
    vec3 V = normalize(pfd.viewPos.xyz - fragPos);

    // Accumulate radiance
    vec3 Lo = vec3(0.0);

    // 1. Directional Light
    {
        vec3 L = normalize(-lightData.dirLight.direction.xyz);
        vec3 radiance = lightData.dirLight.colorAndIntensity.xyz * lightData.dirLight.colorAndIntensity.w;
        
        vec3 directLighting = ComputePBR(N, V, L, radiance, albedo, roughness, metallic);
        
        // Shadow mapping
        vec4 fragPosLightSpace = pfd.lightSpaceMatrix * vec4(fragPos, 1.0);
        float shadow = ShadowCalculation(fragPosLightSpace, N, L);
        
        Lo += (1.0 - shadow) * directLighting;
    }

    // 2. Point Lights
    for (int i = 0; i < lightData.numPointLights; ++i) {
        vec3 L = normalize(lightData.pointLights[i].positionAndRadius.xyz - fragPos);
        float distance = length(lightData.pointLights[i].positionAndRadius.xyz - fragPos);
        float radius = lightData.pointLights[i].positionAndRadius.w;
        
        if (distance < radius) {
            vec3 radiance = lightData.pointLights[i].colorAndIntensity.xyz * lightData.pointLights[i].colorAndIntensity.w;
            // Attenuation: smooth quadratic falloff
            float attenuation = pow(max(1.0 - (distance / radius), 0.0), 2.0);
            radiance *= attenuation;
            
            Lo += ComputePBR(N, V, L, radiance, albedo, roughness, metallic);
        }
    }

    // 3. Ambient
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    vec3 color = ambient + Lo;

    FragColor = vec4(color, 1.0);
}
