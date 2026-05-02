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

vec3 redPulse(float time) {
    float r = sin(time) * 0.5 + 0.5;
    return vec3(r, 0.0, 0.0);
}

void main() {
    //vec2 uv = gl_FragCoord.xy / iResolution.xy;

    outColor = vec4(redPulse(iTime), 1.0);
}
