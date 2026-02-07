#version 440

layout(location = 0) in uvec3 quadVertex;
layout(location = 1) in uvec2 packed;

layout(location = 0) out vec4 v_color;
layout(location = 1) flat out uint v_face;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
};

layout(std140, binding = 1) uniform buf2 {
    uint relativeChunkPos;
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
    uint rlx, rly;
    uint drlx, drly;
};

VertexData unpackVertex(uvec2 v)
{
    VertexData outv;

    uint low  = v.x;
    uint high = v.y;

    // High word packing (CPU: chunkmeshgeneration.h):
    //   id:     16 bits (bits 0-15)
    //   rlx:     5 bits (bits 16-20)
    //   drlx:    3 bits (bits 21-23)
    //   rly:     5 bits (bits 24-28)
    //   drly:    3 bits (bits 29-31)
    outv.id = extractBits(high, 0u, 16u);
    outv.rlx = extractBits(high, 16u, 5u);
    outv.drlx = extractBits(high, 21u, 3u);
    outv.rly = extractBits(high, 24u, 5u);
    outv.drly = extractBits(high, 29u, 3u);

    // Low word packing (CPU: chunkmeshgeneration.h):
    //   face:    3 bits (bits 0-2)  [0-5: x+,y+,z+,x-,y-,z-]
    //   dx:      3 bits (bits 3-5)
    //   x:       6 bits (bits 6-11)
    //   dy:      3 bits (bits 12-14)
    //   y:       6 bits (bits 15-20)
    //   dz:      3 bits (bits 21-23)
    //   z:       6 bits (bits 24-29)
    outv.face = extractBits(low, 0u, 3u);
    outv.dx = extractBits(low, 3u, 3u);
    outv.x  = extractBits(low, 6u, 6u);
    outv.dy = extractBits(low, 12u, 3u);
    outv.y  = extractBits(low, 15u, 6u);
    outv.dz = extractBits(low, 21u, 3u);
    outv.z  = extractBits(low, 24u, 6u);

    return outv;
}

vec3 decodePosition(VertexData v)
{
    vec3 base = vec3(v.x, v.y, v.z);
    vec3 detail = vec3(v.dx, v.dy, v.dz) / 8.0;
    return base + detail;
}

vec2 decodeRunLength(VertexData v)
{
    vec2 intPart = vec2(v.rlx, v.rly);
    vec2 fracPart = vec2(v.drlx, v.drly) / 8.0;
    return intPart + fracPart;
}

vec3 faceOffset(uint face, vec2 uv)
{
    // Face mapping: 0=x+, 1=y+, 2=z+, 3=x-, 4=y-, 5=z-
    if (face == 0u) {
        return vec3(0.0, uv.x, uv.y); // x+ : yz plane at max x
    }
    if (face == 1u) {
        return vec3(uv.y, 0.0, uv.x); // y+ : xz plane at max y
    }
    if (face == 2u) {
        return vec3(uv.x, uv.y, 0.0); // z+ : xy plane at max z
    }
    if (face == 3u) {
        return vec3(0.0, uv.y, uv.x); // x- : yz plane at min x (swap uv for winding)
    }
    if (face == 4u) {
        return vec3(uv.x, 0.0, uv.y); // y- : xz plane at min y (swap uv for winding)
    }
    return vec3(uv.y, uv.x, 0.0); // z- : xy plane at min z (swap uv for winding)
}

ivec3 unpackRelativeChunkPos(uint packed)
{
    int relX = int(extractBits(packed, 0u, 10u));
    int relY = int(extractBits(packed, 10u, 10u));
    int relZ = int(extractBits(packed, 20u, 10u));
    if (relX >= 512) relX -= 1024;
    if (relY >= 512) relY -= 1024;
    if (relZ >= 512) relZ -= 1024;
    return ivec3(relX, relY, relZ);
}


void main()
{
    VertexData v = unpackVertex(packed);
    vec3 basePos = decodePosition(v);
    vec2 runLength = decodeRunLength(v);
    vec2 quadUv = vec2(quadVertex.xy) * runLength;
    vec3 position = basePos + faceOffset(v.face, quadUv);

    // m_blockDefinitions.insert({0, BlockDefinition{"air"}});
    // m_blockDefinitions.insert({1, BlockDefinition{"stone"}});
    // m_blockDefinitions.insert({2, BlockDefinition{"grass"}});
    // m_blockDefinitions.insert({3, BlockDefinition{"dirt"}});
    // m_blockDefinitions.insert({4, BlockDefinition{"cobblestone"}});
    // m_blockDefinitions.insert({5, BlockDefinition{"planks"}});

    if(v.id == 1) { // stone should be grey
        v_color = vec4(0.5, 0.5, 0.5, 1.0);
    } else if(v.id == 2) { // grass should be green
        v_color = vec4(0.0, 1.0, 0.0, 1.0);
    } else if(v.id == 3) { // dirt should be brown
        v_color = vec4(0.5, 0.25, 0.0, 1.0);
    } else if(v.id == 4) { // cobblestone should be grey
        v_color = vec4(0.5, 0.5, 0.5, 1.0);
    } else if(v.id == 5) { // planks should be brown
        v_color = vec4(0.5, 0.25, 0.0, 1.0);
    } else {
        v_color = vec4(1.0, 1.0, 1.0, 1.0);
    }

    v_face = v.face;

    ivec3 relChunkPos = unpackRelativeChunkPos(relativeChunkPos);
    vec3 worldPos = position + vec3(relChunkPos) * 32.0;
    gl_Position = mvp * vec4(worldPos, 1.0);
}

/*
void main() {
    vec2 pos = vec2(float(gl_VertexIndex % 2) * 2.0 - 1.0,  // x: -1 or 1
                    float(gl_VertexIndex / 2) * 2.0 - 1.0); // y: -1 or 1
    gl_Position = mvp * vec4(quadVertex.xy, 0.0, 1.0);
    v_color = vec4(1.0, 0.0, 0.0, 1.0); // Red
}
*/
