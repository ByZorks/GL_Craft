#version 330 core

layout(location = 0) out vec4 color;

in vec2 v_texCoord;
flat in vec3 v_normal;

uniform sampler2D u_Texture;

void main() {
    vec4 texColor = texture(u_Texture, v_texCoord);

//    float waterStart = 12.0 / 15.0;
//    float waterEnd = 1;
//
//    if (v_texCoord.x >= waterStart && v_texCoord.x <= waterEnd) {
//        texColor.a = 0.7;
//    }

    float lighting = 1.0;
    if (v_normal == vec3(0.0, 1.0, 0.0)) {
        lighting = 1.0; // Top face is fully lit
    } else if (v_normal == vec3(0.0, -1.0, 0.0)) {
        lighting = 0.4; // Bottom face is dimly lit
    } else {
        lighting = 0.7; // Side faces are moderately lit
    }
    vec3 shaded = texColor.rgb * lighting;

    color = vec4(shaded, texColor.a);
}