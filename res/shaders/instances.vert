#version 460 core

layout(location = 0) in uvec3 position; // Vertex position
layout(location = 1) in uvec2 texIndex; // Texture column and row for atlas mapping (normalized)
layout(location = 2) in uint face; // Face index
layout(location = 3) in uint AO; // Ambient Occlusion values (0-3)
layout(location = 4) in ivec3 instancePos; // Instance position in world space

out vec2 v_texCoord;

uniform mat4 u_MVP;

void main() {
    const vec3 worldPos = ivec3(position) + instancePos;
    gl_Position = u_MVP * vec4(worldPos, 1.0);

    const float tileSize = 1.f / 5.f;
    v_texCoord = texIndex * tileSize;
}