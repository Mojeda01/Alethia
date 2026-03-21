#version 450

layout(set = 0, binding = 0) uniform MVP {
    mat4 view;
    mat4 projection;
    vec4 lightPos;
    vec4 viewPos;
} mvp;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 vColor;

void main() {
    gl_Position = mvp.projection * mvp.view * vec4(inPosition, 1.0);
    vColor = inColor;
}
