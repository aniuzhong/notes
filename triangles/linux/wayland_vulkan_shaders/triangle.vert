#version 450

layout(location = 0) in vec3 inPos;
layout(location = 0) out vec3 vColor;

void main() {
    // Vulkan NDC: +Y points down the framebuffer; negate to match OpenGL-style demos.
    gl_Position = vec4(inPos.x, -inPos.y, inPos.z, 1.0);
    vColor = inPos * 0.5 + 0.5;
}
