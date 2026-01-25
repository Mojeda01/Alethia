#version 450

layout(push_constant) uniform Push {
    layout(offset = 0) float timeSeconds;
    layout(offset = 4) float deltaSeconds;
    layout(offset = 8) uint frameIndex;
    layout(offset = 12) float pad;
} pc;

layout(location = 0) out vec3 vColor;

void main() {
    const vec2 positions[3] = vec2[3] (
        vec2(0.0, -0.5),
        vec2(0.5, 0.5),
        vec2(-0.5, 0.5)
    );
    const vec3 colors[3] = vec3[3](
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 0.0, 1.0)
    );

    float a = pc.timeSeconds;
    mat2 r = mat2(cos(a), -sin(a), sin(a), cos(a));
    vec2 p = r * positions[gl_VertexIndex];
    gl_Position = vec4(p, 0.0, 1.0);

    vColor = colors[gl_VertexIndex] * (0.6 + 0.4 * sin(a + vec3(0.0, 2.0, 4.0)));
}
