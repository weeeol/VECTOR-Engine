cbuffer Constants : register(b0) {
    matrix model;
    matrix projection;
    float4 spriteColor;
    int useTexture;
    int3 padding;
};

Texture2D spriteTexture : register(t0);
SamplerState smp : register(s0);

struct VSInput {
    float2 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    float4 worldPos = mul(model, float4(input.position, 0.0f, 1.0f));
    output.position = mul(projection, worldPos);
    output.texCoord = input.texCoord;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float4 color = spriteColor;
    if (useTexture != 0) {
        // Textures might use alpha
        float4 texColor = spriteTexture.Sample(smp, input.texCoord);
        // If it's a text texture, maybe we just multiply
        color *= texColor;
    }
    return color;
}
