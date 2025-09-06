#version 460 core

layout(location = 0) out vec4 color;

in vec2 v_texCoord;
flat in uint v_texLayer;
flat in float v_lightLevel;

uniform sampler2DArray u_TextureArray;

void main() {
    vec4 texColor = texture(u_TextureArray, vec3(v_texCoord, v_texLayer));
    if (texColor.a < 0.1) discard;
    texColor.rgb /= texColor.a; // Un-premultiply alpha

    vec3 shaded = texColor.rgb * 0.7 * v_lightLevel; // Default side lighting

    color = vec4(shaded, texColor.a);
}