#version 450
layout (location = 0) in vec3 FragPos;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoords;
layout (location = 3) in vec4 FragPosLightSpace;

layout (location = 0) out vec4 FragColor;

// Set 0: PerFrameData and LightData
layout(set = 0, binding = 0) uniform PerFrameData {
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMatrix;
    vec4 viewPos;
    vec4 sunDir;
    vec4 sunColor;
    vec4 lightPos;
    vec4 lightColor;
} pfd;

#define MAX_POINT_LIGHTS 64

struct PointLightData {
    vec4 positionAndRadius;
    vec4 colorAndIntensity;
};

struct DirectionalLightData {
    vec4 direction;
    vec4 colorAndIntensity;
};

layout(set = 0, binding = 1) uniform LightDataBlock {
    DirectionalLightData dirLight;
    PointLightData pointLights[MAX_POINT_LIGHTS];
    int numPointLights;
} lightData;

// Set 0, binding 2: Shadow map sampler
layout(set = 0, binding = 2) uniform sampler2D shadowMap;

// Set 1: Material Textures
layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D aoMap;

// Push Constants
layout(push_constant) uniform MaterialData {
    layout(offset = 64) vec4 albedoColor;
    uint hasAlbedoMap;
    uint hasNormalMap;
    uint hasMetallicRoughnessMap;
    uint hasAOMap;
    float metallicFactor;
    float roughnessFactor;
    uint isUnlit;
    uint _padding;
} mat;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0, 1] range for UVs (Vulkan Z is already 0 to 1)
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    
    // Check if outside shadow map bounds
    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;
        
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
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

vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(FragPos);
    vec3 Q2  = dFdy(FragPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() {
    vec4 albedoTex = mat.hasAlbedoMap != 0 ? texture(albedoMap, TexCoords) : vec4(1.0);
    vec3 albedo = pow(albedoTex.rgb * mat.albedoColor.rgb, vec3(2.2));
    float alpha = albedoTex.a * mat.albedoColor.a;
    
    if (mat.isUnlit != 0) {
        FragColor = vec4(albedo, alpha);
        return;
    }

    float metallic = mat.metallicFactor;
    float roughness = mat.roughnessFactor;
    if (mat.hasMetallicRoughnessMap != 0) {
        vec4 mr = texture(metallicRoughnessMap, TexCoords);
        metallic *= mr.b;
        roughness *= mr.g;
    }

    float ao = mat.hasAOMap != 0 ? texture(aoMap, TexCoords).r : 1.0;

    vec3 N = (mat.hasNormalMap != 0) ? getNormalFromMap() : normalize(Normal);
    vec3 V = normalize(pfd.viewPos.xyz - FragPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    // Directional light
    {
        vec3 L = normalize(-lightData.dirLight.direction.xyz);
        vec3 H = normalize(V + L);
        vec3 radiance = lightData.dirLight.colorAndIntensity.xyz * lightData.dirLight.colorAndIntensity.w;

        float NDF = DistributionGGX(N, H, roughness);       
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  

        float NdotL = max(dot(N, L), 0.0);        
        
        float shadow = ShadowCalculation(FragPosLightSpace, N, L);
        Lo += (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Point lights (if any)
    for(int i = 0; i < lightData.numPointLights; ++i) 
    {
        vec3 lightPos = lightData.pointLights[i].positionAndRadius.xyz;
        vec3 L = normalize(lightPos - FragPos);
        vec3 H = normalize(V + L);
        
        float distance = length(lightPos - FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightData.pointLights[i].colorAndIntensity.xyz * lightData.pointLights[i].colorAndIntensity.w * attenuation;
        
        float NDF = DistributionGGX(N, H, roughness);       
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  

        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Ambient lighting (we'll replace this with IBL later)
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    vec3 color = ambient + Lo;

    FragColor = vec4(color, alpha);
}
