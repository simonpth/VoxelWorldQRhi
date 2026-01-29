#version 440

layout(location = 0) in vec4 v_color;
layout(location = 1) flat in uint v_face;

layout(location = 0) out vec4 fragColor;

vec3 faceColors[6] = vec3[](
    vec3(1.00, 0.35, 0.00), // orange
    vec3(0.00, 0.75, 0.45), // teal-green
    vec3(0.20, 0.40, 0.95), // strong blue
    vec3(0.95, 0.90, 0.25), // lime-yellow
    vec3(0.75, 0.25, 0.90), // violet
    vec3(0.15, 0.85, 0.95)  // cyan
);


void main()
{
    fragColor = vec4(faceColors[v_face], 1.0);
}
