#version 450
layout(location = 0) in vec2 inTexCoords;

layout(location = 0) out vec4 outColor; // R channel for occlusion

layout(set = 0, binding = 0) uniform sampler2D texNormal;
layout(set = 0, binding = 1) uniform sampler2D texDepth;
layout(set = 0, binding = 2) uniform sampler2D texNoise;

layout(set = 0, binding = 3) uniform SSAOData {
    vec4 samples[64];
    float radius;
    float bias;
    vec2 screenSize;
} ssao;

layout(push_constant) uniform PushConstants {
    mat4 projection;
} pc;

// Helper function to calculate View-Space Position from Depth
vec3 ReconstructViewPosition(vec2 texCoords, float depth) {
    // Convert depth to NDC [-1, 1] (Vulkan depth is 0 to 1, but projection expects -1 to 1 or 0 to 1 depending on setup. Assuming standard Vulkan 0 to 1)
    // Actually, in Vulkan NDC Z is 0 to 1.
    // X, Y are -1 to 1.
    vec4 ndc = vec4(
        texCoords.x * 2.0 - 1.0,
        texCoords.y * 2.0 - 1.0,
        depth,
        1.0
    );

    vec4 viewPos = inverse(pc.projection) * ndc;
    return viewPos.xyz / viewPos.w;
}

void main() {
    float depth = texture(texDepth, inTexCoords).r;
    // Don't calculate SSAO for skybox (depth = 1.0)
    if (depth == 1.0) {
        outColor = vec4(1.0);
        return;
    }

    vec3 fragPos = ReconstructViewPosition(inTexCoords, depth);
    vec3 normal = normalize(texture(texNormal, inTexCoords).rgb);

    // Tile noise texture over screen based on screen dimensions divided by noise size
    vec2 noiseScale = ssao.screenSize / 4.0;
    vec3 randomVec = normalize(texture(texNoise, inTexCoords * noiseScale).xyz);

    // Create TBN matrix
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    int kernelSize = 64;

    for (int i = 0; i < kernelSize; ++i) {
        // get sample position
        vec3 samplePos = TBN * ssao.samples[i].xyz; // from tangent to view-space
        samplePos = fragPos + samplePos * ssao.radius;

        // project sample position (to sample texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = pc.projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        // transform to range 0.0 - 1.0
        offset.x = offset.x * 0.5 + 0.5;
        offset.y = offset.y * 0.5 + 0.5;

        // get sample depth
        float sampleDepth = texture(texDepth, offset.xy).r;
        vec3 sampleViewPos = ReconstructViewPosition(offset.xy, sampleDepth);

        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, ssao.radius / abs(fragPos.z - sampleViewPos.z));
        occlusion += (sampleViewPos.z >= samplePos.z + ssao.bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(kernelSize));
    outColor = vec4(occlusion, occlusion, occlusion, 1.0);
}
