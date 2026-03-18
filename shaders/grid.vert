#version 450

layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightPos;
    vec4 viewPos;
} mvp;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out float vDist;

void main() {
    vec4 worldPos = vec4(inPosition, 1.0);
    gl_Position = mvp.projection * mvp.view * worldPos;

    vWorldPos = worldPos.xyz;
    vDist = length(mvp.viewPos.xyz - worldPos.xyz);
}
