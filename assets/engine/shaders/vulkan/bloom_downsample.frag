#version 450

layout (location = 0) in vec2 TexCoords;
layout (location = 0) out vec4 FragColor;

layout (set = 0, binding = 0) uniform sampler2D srcTexture;

layout (push_constant) uniform PushConstants {
    vec2 srcResolution;
    float threshold;
} pc;

void main() {
    vec2 texelSize = 1.0 / pc.srcResolution;
    float x = texelSize.x;
    float y = texelSize.y;

    vec3 a = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y + 2*y)).rgb;
    vec3 b = texture(srcTexture, vec2(TexCoords.x,       TexCoords.y + 2*y)).rgb;
    vec3 c = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y + 2*y)).rgb;

    vec3 d = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y)).rgb;
    vec3 e = texture(srcTexture, vec2(TexCoords.x,       TexCoords.y)).rgb;
    vec3 f = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y)).rgb;

    vec3 g = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y - 2*y)).rgb;
    vec3 h = texture(srcTexture, vec2(TexCoords.x,       TexCoords.y - 2*y)).rgb;
    vec3 i = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y - 2*y)).rgb;

    vec3 j = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y + y)).rgb;
    vec3 k = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y + y)).rgb;
    vec3 l = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y - y)).rgb;
    vec3 m = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y - y)).rgb;

    vec3 downsample = e * 0.125;
    downsample += (a + c + g + i) * 0.03125;
    downsample += (b + d + f + h) * 0.0625;
    downsample += (j + k + l + m) * 0.125;

    if (pc.threshold > 0.0) {
        float brightness = dot(downsample, vec3(0.2126, 0.7152, 0.0722));
        if (brightness < pc.threshold) {
            downsample = vec3(0.0);
        } else {
            float knee = pc.threshold * 0.5;
            float soft = brightness - pc.threshold + knee;
            soft = clamp(soft / (2.0 * knee + 0.00001), 0.0, 1.0);
            soft = soft * soft;
            downsample *= soft;
        }
    }

    FragColor = vec4(downsample, 1.0);
}
