#version 450

layout(push_constant) uniform Push {
    float time;
} push;

layout(location = 0) out vec3 fragColor;

void main()
{
    vec2 positions[3];
    positions[0] = vec2( 0.0, -0.5);
    positions[1] = vec2( 0.5,  0.5);
    positions[2] = vec2(-0.5,  0.5);

    vec3 colors[3];
    colors[0] = vec3(1.0, 0.0, 0.0);
    colors[1] = vec3(0.0, 1.0, 0.0);
    colors[2] = vec3(0.0, 0.0, 1.0);

    vec2 pos = positions[gl_VertexIndex];
    pos += vec2(sin(push.time * 4.0 + float(gl_VertexIndex)), cos(push.time * 3.0)) * 0.15;
    gl_Position = vec4(pos, 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
