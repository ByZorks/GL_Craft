#version 460 core

struct BlockVertex {
    uvec3 position;// Vertex position in world space
    uvec2 texIndex;// Texture column and row for atlas mapping (normalized)
    uint face;// Face index (0-5 for 6 faces)
    uvec4 AO;// Ambient Occlusion values for each vertex (0-3)
};

layout(std430, binding = 1) readonly buffer blockVertexPullData {
    uvec2 packedVertices[];
};

layout(std140, binding = 0) uniform MVP {
    mat4 u_MVP; // Model-View-Projection matrix
};

out vec2 v_texCoord;
flat out uint v_face;
out float v_AO;
out float v_refractionFactor;

uniform vec3 u_CameraPos;
uniform float u_Time;
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

BlockVertex unpackVertexData(uvec2 packedData) {
    BlockVertex v;

    v.position.x = (packedData.x >> 0)  & 0x1Fu;  // 5 bits
    v.position.y = (packedData.x >> 5)  & 0x1Fu;
    v.position.z = (packedData.x >> 13) & 0x1Fu;
    v.face       = (packedData.x >> 18) & 0x7u;   // 3 bits

    v.texIndex.x = (packedData.y >> 0)  & 0xFu;   // 4 bits
    v.texIndex.y = (packedData.y >> 4)  & 0xFu;
    v.AO.x       = (packedData.y >> 8)  & 0x3u;   // 2 bits
    v.AO.y       = (packedData.y >> 10) & 0x3u;
    v.AO.z       = (packedData.y >> 12) & 0x3u;
    v.AO.w       = (packedData.y >> 14) & 0x3u;

    return v;
}

void main() {
    // Pull data from the buffer
    const int index = gl_VertexID / 6;
    const int currentVertexID = gl_VertexID % 6;
    const uvec2 packedData = packedVertices[index];
    const BlockVertex data = unpackVertexData(packedData);

    // Face index
    v_face = data.face;

    // Position and offset calculation
    const int quadVertexIndex = indices[currentVertexID];
    const vec3 offset = faceOffsets[data.face][quadVertexIndex];
    vec3 worldPos = vec3(data.position) + offset + u_Offset;
    // The 2 first vertex drawn are the top vertices
    bool isTopVertex = (data.face == 4u || data.face == 6u) || (data.face < 4u && (gl_VertexID % 4 < 2));
    if (isTopVertex) {
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
    v_AO = AO_f == 0.0 ? 0.33 : AO_f; // Prevent completely dark faces
}