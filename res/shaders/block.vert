#version 460 core

layout(location = 0) in vec3 position; // Vertex position in local chunk space
layout(location = 1) in vec2 texIndex; // Texture column and row for atlas mapping
layout(location = 2) in vec3 normal; // Normal vector for lighting calculations

out vec2 v_texCoord;
flat out vec3 v_normal;

uniform vec3 u_ChunkOffset;
uniform mat4 u_MVP;

void main() {
    const vec3 worldPos = position + u_ChunkOffset;
    gl_Position = u_MVP * vec4(worldPos, 1.0);

    const float tileSize = 255.f / 4.f; // Map 0-255 to 0-1 range for texture atlas, 4 tiles per row/column
    v_texCoord = texIndex * vec2(tileSize, tileSize);

    v_normal = normal;
}