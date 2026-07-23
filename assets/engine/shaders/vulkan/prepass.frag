#version 450
layout(location = 0) in vec3 ViewNormal;
layout(location = 1) in vec3 ViewPos;

layout(location = 0) out vec4 outNormal;
layout(location = 1) out vec4 outPosition;

void main() {
    outNormal = vec4(normalize(ViewNormal), 1.0);
    outPosition = vec4(ViewPos, 1.0);
}
