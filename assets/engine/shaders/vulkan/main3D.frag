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

// Set 1: Material Texture
layout(set = 1, binding = 0) uniform sampler2D diffuseMap;

// Push Constants
layout(push_constant) uniform MaterialData {
    layout(offset = 64) vec4 albedoColor;
    uint hasDiffuseMap;
    uint isUnlit;
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

void main() {
    vec4 color = mat.albedoColor;
    if (mat.hasDiffuseMap != 0) {
        color *= texture(diffuseMap, TexCoords);
    }
    
    if (mat.isUnlit != 0) {
        FragColor = color;
        return;
    }
    
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(pfd.viewPos.xyz - FragPos);
    
    float matSpecStr = 0.5;
    float matShininess = 32.0;

    // Ambient
    vec3 totalLighting = 0.3 * lightData.dirLight.colorAndIntensity.xyz;

    // Directional Light
    totalLighting += CalcDirectionalLight(lightData.dirLight, norm, viewDir, matSpecStr, matShininess);

    vec3 result = totalLighting * color.rgb;
    FragColor = vec4(result, color.a);
}
