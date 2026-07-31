struct VSOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

VSOutput VSMain(uint vertexID : SV_VertexID) {
    VSOutput output;
    
    // Generate a fullscreen triangle (covers the whole screen)
    output.texcoord = float2((vertexID << 1) & 2, vertexID & 2);
    
    // Map texcoord (0,0) to top-left (-1,1) in clip space, and (2,2) to bottom-right (3,-3)
    output.position = float4(output.texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    
    return output;
}
