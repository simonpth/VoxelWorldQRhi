#version 440

layout(location = 0) in vec4 v_color;
layout(location = 1) flat in uint v_face;

layout(location = 0) out vec4 fragColor;

vec3 faceColors[6] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 1.0, 0.0),
    vec3(1.0, 0.0, 1.0),
    vec3(0.0, 1.0, 1.0)
);

void main()
{
    fragColor = vec4(faceColors[v_face], 1.0);
}
