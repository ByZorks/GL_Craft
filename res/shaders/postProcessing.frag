#version 460

layout(location = 0) out vec4 color;

in vec2 v_texCoord;

uniform sampler2D u_SceneTexture;
uniform sampler2D u_DepthTexture;
uniform bool u_IsUnderWater;

const float fogNear = 0.9;
const float fogFar = 1.0;

void main() {
    const vec4 sceneColor = texture(u_SceneTexture, v_texCoord);
    const float depth = texture(u_DepthTexture, v_texCoord).r;

    if (depth >= 1.0 || !u_IsUnderWater) {
        color = sceneColor;
        return;
    }

    const float distance = ((fogFar - depth) / (fogFar - fogNear));
    const float fogFactor = clamp(distance, 0.0, 1.0);
    const vec4 fogColor = vec4(0.3, 0.3, 0.9, 1.0);

    color = mix(fogColor, sceneColor, fogFactor);
}