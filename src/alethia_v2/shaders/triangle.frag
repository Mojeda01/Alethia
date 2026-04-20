#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    float time;
} push;

void main()
{
    vec2 uv = gl_FragCoord.xy / vec2(1280.0, 720.0);
    float angle = push.time * 2.0 + length(uv - 0.5) * 8.0;
    vec3 col = 0.5 + 0.5 * cos(angle + vec3(0.0, 2.0, 4.0));
    col *= fragColor;
    outColor = vec4(col, 1.0);
}


