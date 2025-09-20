#version 460 core

layout(location = 0) out vec4 color;

in vec2 v_texCoord;
flat in uint v_texLayer;
flat in uint v_face;
in float v_AO;
flat in float v_sunlightLevel;
flat in vec3 v_blockLightLevel;

uniform sampler2DArray u_TextureArray;

void main() {
    vec4 texColor = texture(u_TextureArray, vec3(v_texCoord, v_texLayer));
    if (texColor.a < 0.1) discard;
    texColor.rgb /= texColor.a; // Un-premultiply alpha

    // Debug
//    int aoLevel = int(floor(v_AO * 3.0 + 0.5));
//    if (aoLevel == 0) { color = vec4(1, 0.984, 0, 1); return; }
//    if (aoLevel == 1) { color = vec4(0, 1, 0.169, 1); return; }
//    if (aoLevel == 2) { color = vec4(0, 0.122, 1, 1); return; }
//    if (aoLevel == 3) { color = vec4(1, 0, 0.953, 1); return; }

    // Face lighting
    float lighting = 1.0;
    if (v_face == 4u) {
        lighting = 1.0; // Top
    } else if (v_face == 5u) {
        lighting = 0.4; // Bottom
    } else {
        lighting = 0.7; // Side
    }

    const vec3 totalLight = max(vec3(v_sunlightLevel), v_blockLightLevel);
    const vec3 shaded = texColor.rgb * lighting * v_AO * totalLight;

    color = vec4(shaded, texColor.a);
}