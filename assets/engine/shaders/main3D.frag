#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec3 sunDir;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;
uniform int hasTexture;
uniform int isUnlit;
uniform mat4 lightSpaceMatrix;

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

void main() {
    if (isUnlit == 1) {
        FragColor = vec4(objectColor, 1.0);
        return;
    }

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 sunColor = vec3(0.9, 0.9, 0.8);
    
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * sunColor;
    
    // Diffuse (Sun)
    float diffSun = max(dot(norm, sunDir), 0.0);
    vec3 diffuseSun = diffSun * sunColor;

    // Specular (Sun)
    float specularStrength = 0.5;
    vec3 halfwayDir = normalize(sunDir + viewDir);  
    float specSun = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specularSun = specularStrength * specSun * sunColor;
    
    float shadow = ShadowCalculation(FragPosLightSpace, norm, sunDir);
    vec3 sunLighting = (ambient + (1.0 - shadow) * (diffuseSun + specularSun));

    // --- TRUE VOLUMETRIC RAYMARCHING ---
    int STEPS = 30;
    vec3 rayVector = FragPos - viewPos;
    float rayLength = length(rayVector);
    vec3 rayDir = rayVector / rayLength;
    float stepSize = rayLength / float(STEPS);
    vec3 stepVec = rayDir * stepSize;
    
    vec3 currentPos = viewPos;
    
    // Dithering to reduce banding artifacts
    float dither = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
    currentPos += stepVec * dither;
    
    float scattering = 0.0;
    
    for(int i = 0; i < STEPS; ++i) {
        vec4 lightSpacePos = lightSpaceMatrix * vec4(currentPos, 1.0);
        vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
        projCoords = projCoords * 0.5 + 0.5;
        
        if(projCoords.z <= 1.0 && projCoords.x >= 0.0 && projCoords.x <= 1.0 && projCoords.y >= 0.0 && projCoords.y <= 1.0) {
            float depth = texture(shadowMap, projCoords.xy).r;
            if(projCoords.z - 0.005 <= depth) {
                scattering += 1.0;
            }
        }
        currentPos += stepVec;
    }
    
    scattering /= float(STEPS);
    
    // Henyey-Greenstein phase function approximation
    float g = 0.6; 
    float cosTheta = dot(rayDir, sunDir);
    float phase = (1.0 - g*g) / (4.0 * 3.14159 * pow(1.0 + g*g - 2.0*g*cosTheta, 1.5));
    
    vec3 volumetricLight = sunColor * scattering * phase * 0.15; // Lower intensity to prevent washout
    // -----------------------------------

    // Point Light
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));
    diffuse *= attenuation;

    vec3 halfwayDirPt = normalize(lightDir + viewDir);  
    float specPt = pow(max(dot(norm, halfwayDirPt), 0.0), 32.0);
    vec3 specularPt = specularStrength * specPt * lightColor * attenuation;

    vec3 baseColor = hasTexture == 1 ? texture(diffuseTexture, TexCoords).rgb * objectColor : objectColor;
    vec3 result = (sunLighting + diffuse + specularPt) * baseColor;
    
    // Add additive volumetric light!
    result += volumetricLight;
    
    FragColor = vec4(result, 1.0);
}
