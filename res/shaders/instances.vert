#version 460 core

struct BlockVertex {
    uvec3 position;// Vertex position in world space
    uvec2 texIndex;// Texture column and row for atlas mapping (normalized)
    uint face;// Face index (0-5 for 6 faces)
    uvec4 AO;// Ambient Occlusion values for each vertex (0-3)
};

layout(std430, binding = 1) readonly buffer blockVertexPullData {
    BlockVertex vertices[];
};

layout (std430, binding = 2) readonly buffer instanceData {
    ivec3 instancePos[]; // Instance position in world space
};

layout(std140, binding = 0) uniform MVP {
    mat4 u_MVP; // Model-View-Projection matrix
};

out vec2 v_texCoord;

const vec3 faceOffsets[2][4] = {
    // FRONT (+Z)
    vec3[4](vec3(0,1,1), vec3(1,1,0), vec3(1,0,0), vec3(0,0,1)),
    // BACK (-Z)
    vec3[4](vec3(1,1,1), vec3(0,1,0), vec3(0,0,0), vec3(1,0,1)),
};

const vec2 texOffsets[4] = vec2[4](
    vec2(0, 1), // Top-left
    vec2(1, 1), // Top-right
    vec2(1, 0), // Bottom-right
    vec2(0, 0)  // Bottom-left
);

const int indices[6] = {0, 2, 1, 0, 3, 2};

void main() {
    // Pull data from the buffer
    const int index = gl_VertexID / 6;
    const int currentVertexID = gl_VertexID % 6;
    const BlockVertex data = vertices[index];

    // Position and offset calculation
    const int quadVertexIndex = indices[currentVertexID];
    const vec3 offset = faceOffsets[data.face][quadVertexIndex];
    const vec3 worldPos = vec3(instancePos[gl_InstanceID]) + offset;
    gl_Position = u_MVP * vec4(worldPos, 1.0);

    // Texture coordinates
    const float tileSize = 1.f / 5.f;
    v_texCoord = (vec2(data.texIndex) + texOffsets[quadVertexIndex]) * tileSize;
}