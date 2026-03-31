#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

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

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(ubo.lightPos.xyz - fragWorldPos);
    vec3 viewDir = normalize(ubo.viewPos.xyz - fragWorldPos);

    // Simple diffuse + specular
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 ambient = 0.2 * fragColor;
    vec3 diffuse = diff * fragColor;
    vec3 specular = spec * vec3(0.5);

    outColor = vec4(ambient + diffuse + specular, 1.0);
}
