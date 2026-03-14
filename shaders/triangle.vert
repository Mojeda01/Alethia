#version 450

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
    float a = pc.timeSeconds;
    mat2 r = mat2(vec2(cos(a), sin(a)), vec2(-sin(a), cos(a))); 
    vec2 p = r * inPosition.xy;
    gl_Position = vec4(p, inPosition.z, 1.0);
    vColor = inColor * (0.6 + 0.4 * sin(a + vec3(0.0, 2.0, 4.0)));
}
