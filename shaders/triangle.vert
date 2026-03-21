#version 450

layout(set = 0, binding = 0) uniform MVP {
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
    layout(offset = 16) mat4 model;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec3 vWorldPos;
layout(location = 3) out float vDist;
layout(location = 4) out vec2 vTexCoord;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0); 
    gl_Position = mvp.projection * mvp.view * worldPos;

    vWorldPos = worldPos.xyz;
    vNormal = mat3(pc.model) * inNormal; 
    vColor = inColor;
    vDist = length(mvp.viewPos.xyz - worldPos.xyz);
    vTexCoord = inTexCoord;
}
