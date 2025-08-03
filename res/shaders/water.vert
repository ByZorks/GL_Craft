#version 460 core

layout(location = 0) in vec3 position; // Vertex position in local chunk space
layout(location = 1) in vec2 texIndex; // Texture column and row for atlas mapping
layout(location = 2) in uint face; // Face index (0-5 for 6 faces)

out vec2 v_texCoord;
flat out uint v_face;

uniform vec3 u_Offset;
uniform mat4 u_MVP;

void main() {
    vec3 adjustedPosition = position;
    if (face == 0 || face == 1 || face == 2 || face == 3 || face == 4) {
        adjustedPosition.y -= .2; // Side and top faces need to be slightly lower
    }
    const vec3 worldPos = adjustedPosition + u_Offset;
    gl_Position = u_MVP * vec4(worldPos, 1.0);

    const float tileSize = 255.f / 4.f; // Map 0-255 to 0-1 range for texture atlas, 4 tiles per row/column
    v_texCoord = texIndex * vec2(tileSize, tileSize);
}