#version 450

layout(push_constant) uniform Push {
    float time;
} push;

vec2 positions[3] = vec2[] (
    vec2( 0.0, -0.5),
    vec2( 0.5,  0.5),
    vec2(-0.5,  0.5)
);

vec3 colors[3] = vec3[] (
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

layout(location = 0) out vec3 fragColor;

void main()
{
    vec2 pos = positions[gl_VertexIndex];
    pos += vec2(sin(push.time * 4.0 + gl_VertexIndex), cos(push.time * 3.0)) * 0.15;
}
