#version 460 core

layout(location = 0) out vec4 color;

in vec2 v_texCoord;
flat in uint v_texLayer;
flat in uint v_face;
in float v_AO;
in float v_refractionFactor;

uniform sampler2DArray u_TextureArray;

void main() {
    vec4 texColor = texture(u_TextureArray, vec3(v_texCoord, v_texLayer));
    if (texColor.a < 0.1) discard;

    float lighting = 1.0;
    if (v_face == 4u) {
        lighting = 1.0; // Top
    } else if (v_face == 4u || v_face == 6u) {
        lighting = 0.4; // Bottom
    } else {
        lighting = 0.7; // Side
    }

    const vec3 shaded = texColor.rgb * lighting * v_AO * v_refractionFactor;

    color = vec4(shaded, texColor.a);
}