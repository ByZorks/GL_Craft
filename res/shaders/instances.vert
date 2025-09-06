#version 460 core

struct BlockVertex {
    uvec3 position;// Vertex position in world space
    uint texLayer; // Texture layer for array texture
    uint face;// Face index (0-5 for 6 faces)
    uvec4 AO;// Ambient Occlusion values for each vertex (0-3)
    uint lightLevel; // Light level (0-15)
};

// UBOs
layout(std140, binding = 0) uniform MVP {
    mat4 u_MVP; // Model-View-Projection matrix
};

layout(std140, binding = 1) uniform Time {
    float u_Time;
};

// SSBOs
layout(std430, binding = 0) readonly buffer blockVertexPullData {
    uvec2 packedVertices[];
};

layout (std430, binding = 1) readonly buffer instanceData {
    int instancePos[]; // Instance position in world space
};

out vec2 v_texCoord;
flat out uint v_texLayer;
flat out float v_lightLevel;

const vec3 faceOffsets[8] = {
    // FRONT (+Z)
    vec3(0,1,1), vec3(1,1,0), vec3(1,0,0), vec3(0,0,1),
    // BACK (-Z)
    vec3(1,1,1), vec3(0,1,0), vec3(0,0,0), vec3(1,0,1)
};

const vec2 texOffsets[4] = {
    vec2(0, 1), // Top-left
    vec2(1, 1), // Top-right
    vec2(1, 0), // Bottom-right
    vec2(0, 0)  // Bottom-left
};

const int indices[6] = {0, 2, 1, 0, 3, 2};

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

    // LightLevel (4 bits)
    v.lightLevel = (packedData[1] >> 0) & 0xFu;

    return v;
}
void main() {
    // Pull data from the buffer
    const int index = gl_VertexID / 6;
    const int currentVertexID = gl_VertexID % 6;
    const uvec2 packedData = packedVertices[index];
    const BlockVertex data = unpackVertexData(packedData);

    // Position and offset calculation
    const vec3 currentInstancePos = vec3(instancePos[gl_InstanceID * 3], instancePos[gl_InstanceID * 3 + 1], instancePos[gl_InstanceID * 3 + 2]);
    const int quadVertexIndex = indices[currentVertexID];
    const vec3 vertexOffset = faceOffsets[int(data.face) * 4 + quadVertexIndex];
    vec3 worldPos = currentInstancePos + vertexOffset;
    // Top vertex
    if (vertexOffset.y > 0.5) {
        worldPos.x += sin(u_Time * 2.5 + worldPos.x * 1.5 + worldPos.z * 1.0) * 0.05;
        worldPos.z += sin(u_Time * 2.5 + worldPos.x * 1.0 + worldPos.z * 1.5) * 0.05;
    }

    gl_Position = u_MVP * vec4(worldPos, 1.0);

    // Texture layer and coordinates
    v_texCoord = texOffsets[quadVertexIndex];
    v_texLayer = data.texLayer;

    // Light level
    v_lightLevel = float(data.lightLevel) / 15.0;
}