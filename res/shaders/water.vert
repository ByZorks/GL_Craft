#version 460 core

layout(location = 0) in uvec3 position; // Vertex position in local chunk space
layout(location = 1) in uvec2 texIndex; // Texture column and row for atlas mapping (normalized)
layout(location = 2) in uint face; // Face index (0-6 for 6 faces + 1 inversed face)
layout(location = 3) in uint AO; // Ambient Occlusion values (0-3)

layout(std140, binding = 0) uniform MVP {
    mat4 u_MVP; // Model-View-Projection matrix
};

out vec2 v_texCoord;
flat out uint v_face;
out float v_AO;
out float v_refractionFactor;

uniform vec3 u_CameraPos;
uniform float u_Time;
uniform vec3 u_Offset;

void main() {
    vec3 worldPos = position + u_Offset;

    // The 2 first vertex drawn are the top vertices
    bool isTopVertex = (face == 4u || face == 6u) || (face < 4u && (gl_VertexID % 4 < 2));

    if (isTopVertex) {
        worldPos.y -= .2;
        worldPos.y += (sin(u_Time * 2.5 + worldPos.x * 2.0 + worldPos.z * 1.5)
                    + cos(u_Time * 1.5 + worldPos.z * 2.5 + worldPos.x * 1.2)) * 0.05;
    }
    gl_Position = u_MVP * vec4(worldPos, 1.0);

    const vec3 toCamera = normalize(u_CameraPos - worldPos);
    v_refractionFactor = pow(dot(toCamera, vec3(0.0, 1.0, 0.0)), 0.5);

    const float tileSize = 1.f / 5.f;
    vec2 animatedTexIndex = vec2(texIndex);
    float frameOffset = mod(floor(u_Time / 1.25), 8.0);
    if (frameOffset >= 5.0) {
        frameOffset -= 5.0;
        animatedTexIndex.y -= 1.0;
    }
    animatedTexIndex.x += frameOffset;

    v_texCoord = animatedTexIndex * tileSize;

    v_face = face;

    const float AO_f = float(AO) / 3.0; // Normalize AO to 0-1 range
    v_AO = AO_f == 0.0 ? 0.33 : AO_f; // Prevent completely dark faces
}