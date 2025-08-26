#version 460 core

struct BlockVertex {
    uvec3 position;// Vertex position in world space
    uvec2 texIndex;// Texture column and row for atlas mapping (normalized)
    uint face;// Face index (0-5 for 6 faces)
    uvec4 AO;// Ambient Occlusion values for each vertex (0-3)
};

// UBOs
layout(std140, binding = 0) uniform MVP {
    mat4 u_MVP; // Model-View-Projection matrix
};

layout(std140, binding = 1) uniform Time {
    float u_Time;
};

// SSBOs
layout(std430, binding = 1) readonly buffer blockVertexPullData {
    uint packedVertices[];
};

layout(std430, binding = 2) readonly buffer blockOffsetPullData {
    int positionOffset[];
};

out vec2 v_texCoord;
flat out uint v_face;
out float v_AO;
out float v_refractionFactor;

uniform vec3 u_CameraPos;

const vec3 faceOffsets[6][4] = {
    // FRONT (+Z)
    vec3[4](vec3(0,1,1), vec3(1,1,1), vec3(1,0,1), vec3(0,0,1)),
    // BACK (-Z)
    vec3[4](vec3(1,1,0), vec3(0,1,0), vec3(0,0,0), vec3(1,0,0)),
    // LEFT (-X)
    vec3[4](vec3(0,1,0), vec3(0,1,1), vec3(0,0,1), vec3(0,0,0)),
    // RIGHT (+X)
    vec3[4](vec3(1,1,1), vec3(1,1,0), vec3(1,0,0), vec3(1,0,1)),
    // TOP (+Y)
    vec3[4](vec3(0,1,0), vec3(1,1,0), vec3(1,1,1), vec3(0,1,1)),
    // BOTTOM (-Y)
    vec3[4](vec3(0,0,1), vec3(1,0,1), vec3(1,0,0), vec3(0,0,0))
};

const vec2 texOffsets[4] = vec2[4](
    vec2(0, 1),  // Top-left
    vec2(1, 1), // Top-right
    vec2(1, 0), // Bottom-right
    vec2(0, 0) // Bottom-left
);

const int indices[6] = {0, 2, 1, 0, 3, 2};

const int SLOT_SIZE = 1000;

BlockVertex unpackVertexData(uint packedData) {
    BlockVertex v;

    // Postion (15 bits)
    v.position.x = (packedData >> 0)  & 0x1Fu;
    v.position.y = (packedData >> 5)  & 0x1Fu;
    v.position.z = (packedData >> 10) & 0x1Fu;

    // TexIndex (5 bits)
    const uint texIndex = (packedData >> 15) & 0x1Fu;
    v.texIndex.x = texIndex % 5u; // Column in the texture atlas
    v.texIndex.y = texIndex / 5u; // Row in the texture atlas

    // FaceIndex (3 bits)
    v.face = (packedData >> 20) & 0x7u;

    // AO (8 bits)
    v.AO.x = (packedData >> 23) & 0x3u;
    v.AO.y = (packedData >> 25) & 0x3u;
    v.AO.z = (packedData >> 27) & 0x3u;
    v.AO.w = (packedData >> 29) & 0x3u;

    return v;
}

void main() {
    // Pull data from the buffer
    const int quadIndex = gl_VertexID / 6;
    const int currentVertexID = gl_VertexID % 6;
    const uint packedData = packedVertices[gl_BaseInstance * SLOT_SIZE + quadIndex];
    const BlockVertex data = unpackVertexData(packedData);

    // Face index
    v_face = data.face;

    // Position and offset calculation
    const int quadVertexIndex = indices[currentVertexID];
    const vec3 vertexOffset = faceOffsets[data.face][quadVertexIndex];
    vec3 worldPos = vec3(data.position) + vertexOffset + vec3(positionOffset[gl_DrawID * 3], positionOffset[gl_DrawID * 3 + 1], positionOffset[gl_DrawID * 3 + 2]);
    // Top vertex
    if (vertexOffset.y > 0.5) {
        worldPos.y -= .2;
        worldPos.y += (sin(u_Time * 2.5 + worldPos.x * 2.0 + worldPos.z * 1.5)
                    + cos(u_Time * 1.5 + worldPos.z * 2.5 + worldPos.x * 1.2)) * 0.05;
    }
    gl_Position = u_MVP * vec4(worldPos, 1.0);

    // Refraction factor based on camera angle
    const vec3 toCamera = normalize(u_CameraPos - worldPos);
    v_refractionFactor = pow(dot(toCamera, vec3(0.0, 1.0, 0.0)), 0.5);

    // Texture coordinates
    const float tileSize = 1.f / 5.f;
    vec2 animatedTexIndex = (vec2(data.texIndex) + texOffsets[quadVertexIndex]);
    float frameOffset = mod(floor(u_Time / 1.25), 8.0);
    if (frameOffset >= 5.0) {
        frameOffset -= 5.0;
        animatedTexIndex.y -= 1.0;
    }
    animatedTexIndex.x += frameOffset;

    v_texCoord = animatedTexIndex * tileSize;

    const float AO_f = float(data.AO[quadVertexIndex]) / 3.0; // Normalize AO to 0-1 range
    v_AO = max(AO_f, 0.33); // Prevent completely dark faces
}