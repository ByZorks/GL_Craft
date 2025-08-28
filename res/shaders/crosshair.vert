#version 460

layout(location = 0) in vec2 position; // Position of the vertex in normalized device coordinates
layout(location = 1) in uint texLayer; // Texture column and row indices for the crosshair texture

out vec2 v_texCoord;
flat out uint v_texLayer;

uniform float u_AspectRatio;

const vec2 texOffsets[4] = vec2[4](
    vec2(0, 1),  // Top-left
    vec2(1, 1), // Top-right
    vec2(1, 0), // Bottom-right
    vec2(0, 0) // Bottom-left
);

void main() {
    // Position
    gl_Position = vec4(position.x / u_AspectRatio, position.y, 0.0, 1.0);

    // Texture layer and coordinates
    const int currentVertexID = gl_VertexID % 6;
    v_texCoord = texOffsets[currentVertexID];
    v_texLayer = texLayer;
}