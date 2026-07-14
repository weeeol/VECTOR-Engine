#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec4 spriteColor;
uniform bool useTexture;

void main() {
    if (useTexture) {
        vec4 sampled = texture(text, TexCoords);
        color = spriteColor * sampled;
    } else {
        color = spriteColor;
    }
}
