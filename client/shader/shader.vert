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

    // high: id(0-15), face(16-18)
    outv.id = extractBits(high, 0u, 16u);
    outv.face = extractBits(high, 16u, 3u);

    // low: x(20-29), y(10-19), z(0-9)
    // each is 6 bits integer + 4 bits fractional
    uint packX = extractBits(low, 20u, 10u);
    outv.x  = extractBits(packX, 4u, 6u);
    outv.dx = extractBits(packX, 0u, 4u);

    uint packY = extractBits(low, 10u, 10u);
    outv.y  = extractBits(packY, 4u, 6u);
    outv.dy = extractBits(packY, 0u, 4u);

    uint packZ = extractBits(low, 0u, 10u);
    outv.z  = extractBits(packZ, 4u, 6u);
    outv.dz = extractBits(packZ, 0u, 4u);

    return outv;
}

vec3 decodePosition(VertexData v)
{
    vec3 base = vec3(v.x, v.y, v.z);
    vec3 detail = vec3(v.dx, v.dy, v.dz) / 16.0;
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
