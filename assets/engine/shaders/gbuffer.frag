#version 330 core
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedo;
layout (location = 3) out vec4 gMetallicRoughnessAO;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct MaterialData {
    vec4 albedoColor;
    float roughness;
    float metallic;
    int isUnlit;
    int hasTexture;
    sampler2D diffuseTexture;
};
uniform MaterialData material;

void main() {
    // 1. Position
    gPosition = vec4(FragPos, 1.0);

    // 2. Normal
    gNormal = vec4(normalize(Normal), 1.0);

    // 3. Albedo (Alpha stores isUnlit flag: 1.0 = unlit, 0.0 = lit)
    vec3 baseColor = material.albedoColor.rgb;
    if (material.hasTexture != 0) {
        baseColor *= texture(material.diffuseTexture, TexCoords).rgb;
    }
    
    float isUnlitVal = material.isUnlit != 0 ? 1.0 : 0.0;
    gAlbedo = vec4(baseColor, isUnlitVal);

    // 4. Metallic, Roughness, AO (Metallic: R, Roughness: G, AO: B)
    gMetallicRoughnessAO = vec4(material.metallic, material.roughness, 1.0, 1.0);
}
