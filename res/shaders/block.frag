#version 460 core

layout(location = 0) out vec4 color;

in vec2 v_texCoord;
flat in uint v_face;

uniform sampler2D u_Texture;

void main() {
    vec4 texColor = texture(u_Texture, v_texCoord);

    float lighting = 1.0;
    if (v_face == 4u) {
        lighting = 1.0; // Top
    } else if (v_face == 5u) {
        lighting = 0.4; // Bottom
    } else {
        lighting = 0.7; // Side
    }
    vec3 shaded = texColor.rgb * lighting;

    color = vec4(shaded, texColor.a);
}