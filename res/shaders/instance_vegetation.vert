#version 460 core

layout(location = 0) in vec3 position; // Vertex position
layout(location = 1) in vec2 texIndex; // Texture column and row for atlas mapping
layout(location = 2) in uint face; // Face index
layout(location = 3) in vec3 instancePos; // Instance position in world space

out vec2 v_texCoord;
flat out uint v_face;

uniform mat4 u_MVP;

void main() {
    const vec3 worldPos = position + instancePos;
    gl_Position = u_MVP * vec4(worldPos, 1.0);

    const float tileSize = 255.f / 4.f; // Map 0-255 to 0-1 range for texture atlas, 4 tiles per row/column
    v_texCoord = texIndex * vec2(tileSize, tileSize);

    v_face = face;
}