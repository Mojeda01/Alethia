#version 450

layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
} mvp;

layout(push_constant) uniform Push {
    layout(offset = 0) float timeSeconds;
    layout(offset = 4) float deltaSeconds;
    layout(offset = 8) uint frameIndex;
    layout(offset = 12) float pad;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 vColor;

void main() {
    gl_Position = mvp.projection * mvp.view * mvp.model * vec4(inPosition, 1.0);
    float a = pc.timeSeconds;
    vColor = inColor * (0.6 + 0.4 * sin(a + vec3(0.0, 2.0, 4.0)));
}
