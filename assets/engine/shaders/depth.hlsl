cbuffer Constants : register(b0) {
    matrix model;
    matrix view; // unused
    matrix projection; // unused
    matrix lightSpaceMatrix;
};

struct VSInput {
    float3 position : POSITION;
};

struct PSInput {
    float4 position : SV_POSITION;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    float4 worldPos = mul(model, float4(input.position, 1.0f));
    output.position = mul(lightSpaceMatrix, worldPos);
    return output;
}

void PSMain(PSInput input) {
    // Depth is automatically written, no output needed
}
