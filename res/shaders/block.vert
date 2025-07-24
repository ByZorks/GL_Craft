#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;

out vec2 v_texCoord;
flat out vec3 v_normal;

uniform vec3 u_ChunkOffset;
uniform mat4 u_MVP;

void main() {
    vec3 worldPos = position + u_ChunkOffset;
    gl_Position = u_MVP * vec4(worldPos, 1.0);
    v_texCoord = texCoord;
    v_normal = normal;
}