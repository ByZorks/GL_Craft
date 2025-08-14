#version 460

layout(location = 0) out vec4 color;

in vec2 v_texCoord;

uniform sampler2D u_SceneTexture;
uniform sampler2D u_DepthTexture;
uniform bool u_IsUnderWater;
uniform float u_RenderDistance;

const float CHUNK_SIZE = 32.0f;
const float CAMERA_NEAR = 0.1;
const float CAMERA_FAR = 1024.0;
const float WATER_FOG_NEAR = 0.1;
const float WATER_FOG_FAR = 30.0;
const float FOG_FAR = u_RenderDistance - CHUNK_SIZE;
const float FOG_NEAR = max(u_RenderDistance - 4 * CHUNK_SIZE, FOG_FAR - CHUNK_SIZE * 0.5);

float linearDepth(float depth, float near, float far) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main() {
    const vec4 sceneColor = texture(u_SceneTexture, v_texCoord);
    const float depth = texture(u_DepthTexture, v_texCoord).r;

    if (depth >= 1.0 && !u_IsUnderWater) {
        color = sceneColor;
        return;
    }

    const float distance = linearDepth(depth, CAMERA_NEAR, CAMERA_FAR);
    if (u_IsUnderWater) {
        const float fogFactor = clamp((WATER_FOG_FAR - distance) / (WATER_FOG_FAR - WATER_FOG_NEAR), 0.1, 1.0);
        const vec4 fogColor = vec4(0.3, 0.3, 0.9, 1.0);
        color = mix(fogColor, sceneColor, fogFactor);
        return;
    } else {
        const float fogFactor = clamp((FOG_FAR - distance) / (FOG_FAR - FOG_NEAR), 0.0, 1.0);
        const vec4 fogColor = vec4(0.6, 0.7, 1.0, 1.0);
        color = mix(fogColor, sceneColor, fogFactor);
        return;
    }
}