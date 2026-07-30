#version 450
layout (location = 0) in vec2 TexCoords;
layout (location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform sampler2D currentColorMap;
layout(set = 0, binding = 1) uniform sampler2D velocityMap;
layout(set = 0, binding = 2) uniform sampler2D historyColorMap;

// TAA Push Constants
layout(push_constant) uniform PushConstants {
    float blendFactor; // Typically 0.05 to 0.1
} pc;

vec3 RGBToYCoCg(vec3 rgb) {
    float y  =  0.25 * rgb.r + 0.5 * rgb.g + 0.25 * rgb.b;
    float co =  0.5  * rgb.r               - 0.5  * rgb.b;
    float cg = -0.25 * rgb.r + 0.5 * rgb.g - 0.25 * rgb.b;
    return vec3(y, co, cg);
}

vec3 YCoCgToRGB(vec3 ycocg) {
    float r = ycocg.x + ycocg.y - ycocg.z;
    float g = ycocg.x + ycocg.z;
    float b = ycocg.x - ycocg.y - ycocg.z;
    return vec3(r, g, b);
}

void main() {
    vec3 currentColor = texture(currentColorMap, TexCoords).rgb;
    vec2 velocity = texture(velocityMap, TexCoords).xy;

    vec2 historyTexCoords = TexCoords - velocity;

    // If outside screen, return current color (can't use history)
    if (historyTexCoords.x < 0.0 || historyTexCoords.x > 1.0 || historyTexCoords.y < 0.0 || historyTexCoords.y > 1.0) {
        FragColor = vec4(currentColor, 1.0);
        return;
    }

    vec3 historyColor = texture(historyColorMap, historyTexCoords).rgb;

    // Neighborhood Clipping (AABB around current color)
    vec2 texelSize = 1.0 / textureSize(currentColorMap, 0);
    vec3 minColor = currentColor;
    vec3 maxColor = currentColor;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            if (x == 0 && y == 0) continue;
            vec3 neighbor = texture(currentColorMap, TexCoords + vec2(x, y) * texelSize).rgb;
            minColor = min(minColor, neighbor);
            maxColor = max(maxColor, neighbor);
        }
    }

    // Clip history color to bounding box of 3x3 neighborhood
    // Convert to YCoCg for better clipping
    vec3 historyYCoCg = RGBToYCoCg(historyColor);
    vec3 currentYCoCg = RGBToYCoCg(currentColor);
    vec3 minYCoCg = RGBToYCoCg(minColor);
    vec3 maxYCoCg = RGBToYCoCg(maxColor);

    historyYCoCg = clamp(historyYCoCg, minYCoCg, maxYCoCg);
    historyColor = YCoCgToRGB(historyYCoCg);

    // Dynamic blend factor based on velocity
    float velocityLength = length(velocity);
    float blend = mix(pc.blendFactor, 1.0, clamp(velocityLength * 100.0, 0.0, 1.0)); // Increase blend if fast moving to reduce ghosting

    vec3 finalColor = mix(historyColor, currentColor, blend);
    
    FragColor = vec4(finalColor, 1.0);
}
