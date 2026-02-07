#version 440

layout(location = 0) in vec4 v_color;
layout(location = 1) flat in uint v_face;

layout(location = 0) out vec4 fragColor;

vec3 debugFaceColors[6] = vec3[](
    vec3(1.00, 0.35, 0.00), // orange
    vec3(0.00, 0.75, 0.45), // teal-green
    vec3(0.20, 0.40, 0.95), // strong blue
    vec3(0.95, 0.90, 0.25), // lime-yellow
    vec3(0.75, 0.25, 0.90), // violet
    vec3(0.15, 0.85, 0.95)  // cyan
);

vec3 normals[6] = vec3[](
    vec3(1.0, 0.0, 0.0),  // +X
    vec3(0.0, 1.0, 0.0),  // +Y
    vec3(0.0, 0.0, 1.0),  // +Z
    vec3(-1.0, 0.0, 0.0), // -X
    vec3(0.0, -1.0, 0.0), // -Y
    vec3(0.0, 0.0, -1.0)  // -Z
);

void main()
{
    vec3 normal = normals[v_face];
    vec3 lightDir = normalize(vec3(0.2, 1.0, 0.0));
    
    float diff = max(dot(normal, lightDir), 0.0);
    float ambient = 0.6;
    
    // Simple directional lighting
    vec3 col = v_color.rgb * (diff + ambient);
    // vec3 col = debugFaceColors[v_face] * (diff + ambient);

    // Apply some fake AO/shading based on face to make it look more 3D
    float faceShading = 1.0;
    if(v_face == 1) faceShading = 1.0; // Top
    else if(v_face == 4) faceShading = 0.5; // Bottom
    else faceShading = 0.8; // Sides
    
    fragColor = vec4(col * faceShading, 1.0);
}

