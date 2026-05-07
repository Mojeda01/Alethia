#version 450

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    float time;
    float resolutionX;
    float resolutionY;
    float mouseX;
    float mouseY;
    float mousePressed;
} push;

#define iTime push.time
#define iResolution vec3(push.resolutionX, push.resolutionY, 1.0)

void main() {
    // Clean dark background - deep navy/black
    vec3 color = vec3(0.012, 0.015, 0.025);

    // Very subtle vignette for depth.
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    float vignette = 1.0 - length(uv - 0.5) * 0.6;
    color *= vignette * 0.95;

    outColor = vec4(color, 1.0);
}
