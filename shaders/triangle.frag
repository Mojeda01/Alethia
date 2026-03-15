#version 450

layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightPos;
    vec4 viewPos;
} mvp;

layout(push_constant) uniform Push {
    layout(offset = 0) float timeSeconds;
    layout(offset = 4) float deltaSeconds;
    layout(offset = 8) uint frameIndex;
    layout(offset = 12) float pad;
} pc;

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vWorldPos;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(mvp.lightPos.xyz - vWorldPos);

    float ambient = 0.15;
    float diffuse = max(dot(normal, lightDir), 0.0);

    vec3 lit = vColor * (ambient + diffuse);
    outColor = vec4(lit, 1.0);
}
