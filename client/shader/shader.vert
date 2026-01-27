#version 440

layout(location = 0) in uvec2 packed;

layout(location = 0) out vec3 v_color;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
};

void main()
{
    v_color = color;
    gl_Position = mvp * position;
}
