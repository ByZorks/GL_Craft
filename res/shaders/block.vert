#version 460 core

struct BlockVertex {
    uvec3 position;// Vertex position in world space
    uint texLayer; // Texture layer for array texture
    uint face;// Face index (0-5 for 6 faces)
    uvec4 AO;// Ambient Occlusion values for each vertex (0-3)
    uint sunlight; // Sunlight level (0-15)
    uvec3 blockLightRGB; // Block light levels for R, G, B (0-15 each)
};

// UBOs
layout(std140, binding = 0) uniform MVP {
    mat4 u_MVP;// Model-View-Projection matrix
};

// SSBOs
layout(std430, binding = 0) readonly buffer blockVertexPullData {
    uvec2 packedVertices[];
};

layout(std430, binding = 1) readonly buffer blockOffsetPullData {
    int positionOffset[];
};

out vec2 v_texCoord;
flat out uint v_texLayer;
flat out uint v_face;
out float v_AO;
flat out float v_sunlightLevel;
flat out vec3 v_blockLightLevel;

const vec3 faceOffsets[24] = {
    // FRONT (+Z)
    vec3(0,1,1), vec3(1,1,1), vec3(1,0,1), vec3(0,0,1),
    // BACK (-Z)
    vec3(1,1,0), vec3(0,1,0), vec3(0,0,0), vec3(1,0,0),
    // LEFT (-X)
    vec3(0,1,0), vec3(0,1,1), vec3(0,0,1), vec3(0,0,0),
    // RIGHT (+X)
    vec3(1,1,1), vec3(1,1,0), vec3(1,0,0), vec3(1,0,1),
    // TOP (+Y)
    vec3(0,1,0), vec3(1,1,0), vec3(1,1,1), vec3(0,1,1),
    // BOTTOM (-Y)
    vec3(0,0,1), vec3(1,0,1), vec3(1,0,0), vec3(0,0,0)
};

const vec2 texOffsets[4] = {
    vec2(0, 1),  // Top-left
    vec2(1, 1), // Top-right
    vec2(1, 0), // Bottom-right
    vec2(0, 0) // Bottom-left
};

const int indices[6] = {0, 2, 1, 0, 3, 2};

const int SLOT_SIZE = 1000;

BlockVertex unpackVertexData(uvec2 packedData) {
    BlockVertex v;

    // Postion (15 bits)
    v.position.x = (packedData[0] >> 0)  & 0x1Fu;
    v.position.y = (packedData[0] >> 5)  & 0x1Fu;
    v.position.z = (packedData[0] >> 10) & 0x1Fu;

    // TexLayer (6 bits)
    v.texLayer= (packedData[0] >> 15) & 0x3Fu;

    // FaceIndex (3 bits)
    v.face = (packedData[0] >> 21) & 0x7u;

    // AO (8 bits)
    v.AO.x = (packedData[0] >> 24) & 0x3u;
    v.AO.y = (packedData[0] >> 26) & 0x3u;
    v.AO.z = (packedData[0] >> 28) & 0x3u;
    v.AO.w = (packedData[0] >> 30) & 0x3u;

    // LightLevel (16 bits)
    v.sunlight = (packedData[1] >> 0) & 0xFu;
    v.blockLightRGB.x = (packedData[1] >> 4) & 0xFu;
    v.blockLightRGB.y = (packedData[1] >> 8) & 0xFu;
    v.blockLightRGB.z = (packedData[1] >> 12) & 0xFu;

    return v;
}

void main() {
    // Pull data from the buffer
    const int quadIndex = gl_VertexID / 6;
    const int currentVertexID = gl_VertexID % 6;
    const uvec2 packedData = packedVertices[gl_BaseInstance * SLOT_SIZE + quadIndex]; // gl_BaseInstnance is the first slot of the current chunk
    const BlockVertex data = unpackVertexData(packedData);

    // Face index
    v_face = data.face;

    // Position and offset calculation
    const int quadVertexIndex = indices[currentVertexID];
    const vec3 vertexOffset = faceOffsets[int(data.face) * 4 + quadVertexIndex];
    const vec3 worldPos = vec3(data.position) + vertexOffset + vec3(positionOffset[gl_DrawID * 3], positionOffset[gl_DrawID * 3 + 1], positionOffset[gl_DrawID * 3 + 2]);
    gl_Position = u_MVP * vec4(worldPos, 1.0);

    // Texture layer and coordinates
    v_texCoord = texOffsets[quadVertexIndex];
    v_texLayer = data.texLayer;

    // Ambient Occlusion
    const float AO_f = float(data.AO[quadVertexIndex]) / 3.0;// Normalize AO to 0-1 range
    v_AO = max(AO_f, 0.1); // Prevent completely dark faces

    // Light level
    v_sunlightLevel = float(data.sunlight) / 15.0;
    v_blockLightLevel = vec3(data.blockLightRGB) / 15.0;
}