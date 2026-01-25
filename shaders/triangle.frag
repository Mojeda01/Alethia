#version 450

layout(push_constant) uniform Push {
    layout(offset = 0) float timeSeconds;
    layout(offset = 4) float deltaSeconds;
    layout(offset = 8) uint frameIndex;
    layout(offset = 12) float pad;
} pc;

layout(location = 0) in vec3 vColor;
layout(location = 0) out vec4 outColor;

void main() {
    float t = pc.timeSeconds;
    vec3 cycle = 0.5 + 0.5 * sin(t + vec3(0.0, 2.0, 4.0));
    outColor = vec4(vColor * cycle, 1.0);
}
