#version 460 core

layout(location = 0) in vec3 position; // Vertex position in local chunk space
layout(location = 1) in uvec2 texIndex; // Texture column and row for atlas mapping (normalized)
layout(location = 2) in uint face; // Face index (0-5 for 6 faces)

out vec2 v_texCoord;
flat out uint v_face;

uniform float u_Time;
uniform vec3 u_Offset;
uniform mat4 u_MVP;

void main() {
    vec3 worldPos = position + u_Offset;
    worldPos.y -= .2;
    worldPos.y += (sin(u_Time * 2.5 + worldPos.x * 2.0 + worldPos.z * 1.5)
                + cos(u_Time * 1.5 + worldPos.z * 2.5 + worldPos.x * 1.2)) * 0.05;
    gl_Position = u_MVP * vec4(worldPos, 1.0);

    const float tileSize = 1.f / 5.f;
    vec2 animatedTexIndex = vec2(texIndex);
    float frameOffset = mod(floor(u_Time / 1.25), 8.0);
    if (frameOffset >= 5.0) {
        frameOffset -= 5.0;
        animatedTexIndex.y -= 1.0;
    }
    animatedTexIndex.x += frameOffset;

    v_texCoord = animatedTexIndex * tileSize;
}