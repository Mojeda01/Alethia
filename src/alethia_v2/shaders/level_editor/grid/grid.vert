#version 450

void main() {
    vec2 positions[3];
    positions[0] = vec2(-1.0, -1.0);
    positions[1] = vec2( 3.0, -1.0);
    positions[2] = vec2(-1.0,  3.0);

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
