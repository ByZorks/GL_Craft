#version 460 core

layout(location = 0) out vec4 color;

in vec2 v_texCoord;
flat in uint v_face;
in float v_AO;

uniform sampler2D u_Texture;

void main() {
    vec4 texColor = texture(u_Texture, v_texCoord);
    if (texColor.a < 0.1) discard;

    // Debug
//    int aoLevel = int(floor(v_AO * 3.0 + 0.5));
//    if (aoLevel == 0) { color = vec4(1, 0.984, 0, 1); return; }
//    if (aoLevel == 1) { color = vec4(0, 1, 0.169, 1); return; }
//    if (aoLevel == 2) { color = vec4(0, 0.122, 1, 1); return; }
//    if (aoLevel == 3) { color = vec4(1, 0, 0.953, 1); return; }

    float lighting = 1.0;
    if (v_face == 4u) {
        lighting = 1.0; // Top
    } else if (v_face == 5u) {
        lighting = 0.4; // Bottom
    } else {
        lighting = 0.7; // Side
    }
    const vec3 shaded = texColor.rgb * lighting * v_AO;

    color = vec4(shaded, texColor.a);
}