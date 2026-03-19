#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 projection;
} pc;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = pc.projection * pc.view * vec4(inPosition, 1.0);
    fragColor = inColor;
}
