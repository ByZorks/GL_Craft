#version 460

layout(location = 0) in vec2 position; // Position of the vertex in normalized device coordinates
layout(location = 1) in uvec2 texIndex; // Texture column and row indices for the crosshair texture

out vec2 v_texCoord;

uniform float u_AspectRatio;

void main() {
    gl_Position = vec4(position.x / u_AspectRatio, position.y, 0.0, 1.0);

    const float tileSize = 1.f / 5.f;
    v_texCoord = vec2(texIndex) * tileSize;
}