#version 460

layout(location = 0) out vec4 color;

in vec2 v_texCoord;
flat in uint v_texLayer;

uniform sampler2DArray u_TextureArray;

void main() {
    vec4 texColor = texture(u_TextureArray, vec3(v_texCoord, v_texLayer));
    if (texColor.a < 0.1) discard;

    color = texColor;
}