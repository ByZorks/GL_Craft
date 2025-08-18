#version 460 core

layout(location = 0) in uvec3 position; // Vertex position in local chunk space
layout(location = 1) in uvec3 color; // Color for highlighting (RGB)

layout(std140, binding = 0) uniform MVP {
    mat4 u_MVP; // Model-View-Projection matrix
};

flat out uvec3 v_color;

uniform vec3 u_Offset; // Offset for the vertex position

void main() {
    const vec3 worldPos = position + u_Offset;
    gl_Position = u_MVP * vec4(worldPos, 1.0);

    v_color = color;
}
