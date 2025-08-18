#version 460 core

layout(location = 0) in uvec3 position; // Vertex position in local chunk space
layout(location = 1) in uvec2 texIndex; // Texture column and row for atlas mapping (normalized)
layout(location = 2) in uint face; // Face index (0-5 for 6 faces)
layout(location = 3) in uint AO; // Ambient Occlusion values (0-3)

layout(std140, binding = 0) uniform MVP {
    mat4 u_MVP; // Model-View-Projection matrix
};

out vec2 v_texCoord;
flat out uint v_face;
out float v_AO;

uniform vec3 u_Offset;

void main() {
    const vec3 worldPos = position + u_Offset;
    gl_Position = u_MVP * vec4(worldPos, 1.0);

    const float tileSize = 1.f / 5.f;
    v_texCoord = texIndex * tileSize;

    v_face = face;

    const float AO_f = float(AO) / 3.0; // Normalize AO to 0-1 range
    v_AO = AO_f == 0.0 ? 0.1 : AO_f; // Prevent completely dark faces
}