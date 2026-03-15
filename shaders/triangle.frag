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
layout(location = 3) in float vDist;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(mvp.lightPos.xyz - vWorldPos);
    vec3 viewDir = normalize(mvp.viewPos.xyz - vWorldPos);

    // hemisphere ambient
    vec3 skyColor = vec3(0.15, 0.18, 0.25);
    vec3 groundColor = vec3(0.10, 0.08, 0.06);
    float hemi = 0.5 + 0.5 * normal.y;
    vec3 ambient = mix(groundColor, skyColor, hemi);

    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);

    // blinn-phong specular
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specColor = vec3(0.3) * spec;

    // rim light
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    rim = pow(rim, 3.0) * 0.15;

    // combine
    vec3 lit = vColor * (ambient + diff) + specColor + vec3(rim);

    // distance fog
    vec3 fogColor = vec3(0.05, 0.05, 0.08);
    float fogStart = 5000.0;
    float fogEnd = 80000.0;
    float fogFactor = clamp((vDist - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    lit = mix(lit, fogColor, fogFactor);

    // gamma correction
    lit = pow(lit, vec3(1.0 / 2.2));

    outColor = vec4(lit, 1.0);
}
