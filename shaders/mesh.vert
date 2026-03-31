#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 color;
} push; 

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 projection;
    vec4 lightPos;
    vec4 viewPos;
} ubo;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragWorldPos;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    gl_Position = ubo.projection * ubo.view * worldPos;

    fragColor = inColor * push.color;
    fragNormal = mat3(push.model) * inNormal;   // transform normal
    fragTexCoord = inTexCoord;
    fragWorldPos = worldPos.xyz;
}
