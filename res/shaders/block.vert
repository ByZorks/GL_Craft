#version 460 core

layout(location = 0) in vec3 position; // Vertex position in local chunk space
layout(location = 1) in uvec2 texIndex; // Texture column and row for atlas mapping (normalized)
layout(location = 2) in uint face; // Face index (0-5 for 6 faces)

out vec2 v_texCoord;
flat out uint v_face;

uniform vec3 u_Offset;
uniform mat4 u_MVP;

void main() {
    const vec3 worldPos = position + u_Offset;
    gl_Position = u_MVP * vec4(worldPos, 1.0);

    const float tileSize = 1.f / 4.f;
    v_texCoord = texIndex * tileSize;

    v_face = face;
}