#version 440

layout(location = 0) in uvec2 packed;

layout(location = 0) out vec4 v_color;
layout(location = 1) flat out uint v_face;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
};

uint extractBits(uint value, uint shift, uint bits)
{
    return (value >> shift) & ((1u << bits) - 1u);
}

struct VertexData {
    uint id;
    uint face;
    uint x, y, z;
    uint dx, dy, dz;
};

VertexData unpackVertex(uvec2 v)
{
    VertexData outv;

    uint low  = v.x;
    uint high = v.y;

    // upper 32 bits
    outv.id   = high;

    // lower 32 bits
    outv.face = extractBits(low, 29u, 3u);

    outv.x    = extractBits(low, 24u, 5u);
    outv.dx   = extractBits(low, 20u, 4u);

    outv.y    = extractBits(low, 15u, 5u);
    outv.dy   = extractBits(low, 11u, 4u);

    outv.z    = extractBits(low,  6u, 5u);
    outv.dz   = extractBits(low,  2u, 4u);

    return outv;
}

vec3 decodePosition(VertexData v)
{
    vec3 base = vec3(v.x, v.y, v.z);
    vec3 detail = vec3(v.dx, v.dy, v.dz) / 8.0;
    return base + detail;
}



void main()
{
    VertexData v = unpackVertex(packed);
    vec3 position = decodePosition(v);

    // m_blockDefinitions.insert({0, BlockDefinition{"air"}});
    // m_blockDefinitions.insert({1, BlockDefinition{"stone"}});
    // m_blockDefinitions.insert({2, BlockDefinition{"grass"}});
    // m_blockDefinitions.insert({3, BlockDefinition{"dirt"}});
    // m_blockDefinitions.insert({4, BlockDefinition{"cobblestone"}});
    // m_blockDefinitions.insert({5, BlockDefinition{"planks"}});

    if(v.id == 1) {
        v_color = vec4(1.0, 0.0, 1.0, 1.0);
    } else if(v.id == 2) {
        v_color = vec4(0.0, 1.0, 0.0, 1.0);
    } else if(v.id == 3) {
        v_color = vec4(1.0, 0.0, 0.0, 1.0);
    } else if(v.id == 4) {
        v_color = vec4(1.0, 1.0, 0.0, 1.0);
    } else if(v.id == 5) {
        v_color = vec4(1.0, 0.0, 1.0, 1.0);
    }

    v_face = v.face;

    gl_Position = mvp * vec4(position, 1.0);
}
