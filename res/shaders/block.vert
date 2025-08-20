#version 460 core

struct BlockVertex {
    uvec3 position;// Vertex position in world space
    uvec2 texIndex;// Texture column and row for atlas mapping (normalized)
    uint face;// Face index (0-5 for 6 faces)
    uvec4 AO;// Ambient Occlusion values for each vertex (0-3)
};

layout(std430, binding = 1) readonly buffer blockVertexPullData {
    uint packedVertices[];
};

layout(std140, binding = 0) uniform MVP {
    mat4 u_MVP;// Model-View-Projection matrix
};

out vec2 v_texCoord;
flat out uint v_face;
out float v_AO;

uniform vec3 u_Offset;

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
    const int index = gl_VertexID / 6;
    const int currentVertexID = gl_VertexID % 6;
    const uint packedData = packedVertices[index];
    const BlockVertex data = unpackVertexData(packedData);

    // Face index
    v_face = data.face;

    // Position and offset calculation
    const int quadVertexIndex = indices[currentVertexID];
    const vec3 offset = faceOffsets[data.face][quadVertexIndex];
    const vec3 worldPos = vec3(data.position) + offset + u_Offset;
    gl_Position = u_MVP * vec4(worldPos, 1.0);

    // Texture coordinates
    const float tileSize = 1.f / 5.f;
    v_texCoord = (vec2(data.texIndex) + texOffsets[quadVertexIndex]) * tileSize;

    // Ambient Occlusion
    const float AO_f = float(data.AO[quadVertexIndex]) / 3.0;// Normalize AO to 0-1 range
    v_AO = AO_f == 0.0 ? 0.1 : AO_f;// Prevent completely dark faces
}