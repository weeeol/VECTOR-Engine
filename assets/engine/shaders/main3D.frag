#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

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

// Material uniforms (set per material bind)
struct MaterialData {
    vec4 albedoColor;
    float specularStrength;
    float shininess;
    int isUnlit;
    int hasTexture;
    sampler2D diffuseTexture;
};
uniform MaterialData material;

// Legacy uniforms (used by the old DrawMesh path, ignored when material is set)
uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec3 sunDir;
uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;
uniform int hasTexture;
uniform int isUnlit;

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

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0)
        return 0.0;
        
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

vec3 CalcDirectionalLight(DirectionalLightData light, vec3 normal, vec3 viewDir, float matSpecStr, float matShininess) {
    vec3 lightDir = normalize(-light.direction.xyz);
    vec3 lightColor = light.colorAndIntensity.xyz * light.colorAndIntensity.w;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), matShininess);
    
    vec3 diffuse = diff * lightColor;
    vec3 specular = matSpecStr * spec * lightColor;
    
    float shadow = ShadowCalculation(FragPosLightSpace, normal, lightDir);
    return (1.0 - shadow) * (diffuse + specular);
}

vec3 CalcPointLight(PointLightData light, vec3 normal, vec3 viewDir, float matSpecStr, float matShininess) {
    vec3 lightDir = normalize(light.positionAndRadius.xyz - FragPos);
    vec3 lightColor = light.colorAndIntensity.xyz * light.colorAndIntensity.w;
    float radius = light.positionAndRadius.w;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), matShininess);
    
    // Attenuation (inverse square falling off smoothly to 0 at radius)
    float distance = length(light.positionAndRadius.xyz - FragPos);
    if(distance > radius) return vec3(0.0);
    
    float attenuation = pow(max(1.0 - (distance / radius), 0.0), 2.0);
    
    vec3 diffuse = diff * lightColor;
    vec3 specular = matSpecStr * spec * lightColor;
    
    return (diffuse + specular) * attenuation;
}

void main() {
    vec3 baseObjectColor;
    float matSpecStr;
    float matShininess;
    bool matIsUnlit;
    bool matHasTexture;
    vec3 vPos = pfd.viewPos.xyz;

    if (material.albedoColor.a > 0.0) {
        baseObjectColor = material.albedoColor.rgb;
        matSpecStr = material.specularStrength;
        matShininess = material.shininess;
        matIsUnlit = material.isUnlit != 0;
        matHasTexture = material.hasTexture != 0;
    } else {
        baseObjectColor = objectColor;
        matSpecStr = 0.5;
        matShininess = 32.0;
        matIsUnlit = isUnlit != 0;
        matHasTexture = hasTexture != 0;
    }

    vec3 baseColor;
    if (matHasTexture) {
        if (material.albedoColor.a > 0.0) {
            baseColor = texture(material.diffuseTexture, TexCoords).rgb * baseObjectColor;
        } else {
            baseColor = texture(diffuseTexture, TexCoords).rgb * baseObjectColor;
        }
    } else {
        baseColor = baseObjectColor;
    }

    if (matIsUnlit) {
        FragColor = vec4(baseColor, 1.0);
        return;
    }

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(vPos - FragPos);

    // Accumulate lighting
    vec3 totalLighting = vec3(0.0);
    
    // Ambient
    totalLighting += 0.3 * lightData.dirLight.colorAndIntensity.xyz;

    // Directional Light
    totalLighting += CalcDirectionalLight(lightData.dirLight, norm, viewDir, matSpecStr, matShininess);

    // Point Lights
    for (int i = 0; i < lightData.numPointLights; i++) {
        totalLighting += CalcPointLight(lightData.pointLights[i], norm, viewDir, matSpecStr, matShininess);
    }

    vec3 result = totalLighting * baseColor;
    FragColor = vec4(result, 1.0);
}
