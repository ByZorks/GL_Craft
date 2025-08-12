#version 460 core

layout(location = 0) out vec4 color;

in vec2 v_texCoord;

uniform sampler2D u_Texture;

void main() {
    vec4 texColor = texture(u_Texture, v_texCoord);
    if (texColor.a < 0.1) discard;

    vec3 shaded = texColor.rgb * 0.7; // Default side lighting

    color = vec4(shaded, texColor.a);
}