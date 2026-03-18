#version 450

layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightPos;
    vec4 viewPos;
} mvp;

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in float vDist;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 coord = vWorldPos.xz;
    float lineWidth = 0.05;

    float gridX = step(mod(abs(coord.x), 1.0), lineWidth);
    float gridZ = step(mod(abs(coord.y), 1.0), lineWidth);
    float grid = max(gridX, gridZ);

    float bigGridX = step(mod(abs(coord.x), 10.0), lineWidth * 2.0);
    float bigGridZ = step(mod(abs(coord.y), 10.0), lineWidth * 2.0);
    float bigGrid = max(bigGridX, bigGridZ);

    float axisX = step(abs(coord.y), lineWidth * 3.0);
    float axisZ = step(abs(coord.x), lineWidth * 3.0);

    vec3 color = vec3(0.3) * grid;
    color = mix(color, vec3(0.7), bigGrid);
    color = mix(color, vec3(1.0, 0.3, 0.3), axisX);
    color = mix(color, vec3(0.3, 0.3, 1.0), axisZ);

    float visible = max(max(grid, bigGrid), max(axisX, axisZ));

    if (visible < 0.01) discard;

    outColor = vec4(color, 1.0);
}
