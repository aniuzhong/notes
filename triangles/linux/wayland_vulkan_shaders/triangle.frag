#version 450

layout(location = 0) in vec3 vColor;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    float time;
} pc;

void main() {
    outColor = vec4(
        vColor.r + 0.2 * sin(pc.time),
        vColor.g,
        vColor.b + 0.2 * cos(pc.time * 0.7),
        1.0);
}
