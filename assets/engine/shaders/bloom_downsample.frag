#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D srcTexture;
uniform vec2 srcResolution;
uniform float threshold; // Only used on first downsample pass

// 13-tap downsample filter (from Call of Duty: Advanced Warfare presentation)
void main() {
    vec2 texelSize = 1.0 / srcResolution;
    float x = texelSize.x;
    float y = texelSize.y;

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
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

    // Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    // a]b]c   e]  [g]h]i
    //  [j k   [    [l m
    // d]   f  e]  [
    //  [l m   [    [j k
    // g]h]i   e]  [a]b]c
    vec3 downsample = e * 0.125;
    downsample += (a + c + g + i) * 0.03125;
    downsample += (b + d + f + h) * 0.0625;
    downsample += (j + k + l + m) * 0.125;

    // Apply brightness threshold on first pass
    if (threshold > 0.0) {
        float brightness = dot(downsample, vec3(0.2126, 0.7152, 0.0722));
        if (brightness < threshold) {
            downsample = vec3(0.0);
        } else {
            // Soft knee to avoid harsh cutoff
            float knee = threshold * 0.5;
            float soft = brightness - threshold + knee;
            soft = clamp(soft / (2.0 * knee + 0.00001), 0.0, 1.0);
            soft = soft * soft;
            downsample *= soft;
        }
    }

    FragColor = vec4(downsample, 1.0);
}
