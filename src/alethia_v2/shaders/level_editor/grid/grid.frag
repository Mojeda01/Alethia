#version 450

layout(location = 0) out vec4 outColor;

void main() {
    // Hardcoded resolution (change to your window size if you want)
    vec2 resolution = vec2(1280.0, 720.0);
    vec2 uv = (gl_FragCoord.xy - resolution * 0.5) / resolution.y;

    const float major = 10.0;
    const float minor = 50.0;

    vec2 grid = abs(fract(uv * major) - 0.5) / 0.015;
    grid += abs(fract(uv * minor) - 0.5) / 0.08;

    float line = 1.0 - min(grid.x, grid.y);
    float fade = 1.0 - smoothstep(0.0, 30.0, length(uv));

    vec3 color = vec3(0.2, 0.9, 0.3);   // nice green grid

    outColor = vec4(color * line * fade, line * 0.65 * fade);
}
