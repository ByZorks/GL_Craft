#version 330 core

layout(location = 0) out vec4 color;

in vec2 v_texCoord;

uniform vec4 u_Color;
uniform sampler2D u_Texture;

void main() {
    vec4 texColor = texture(u_Texture, v_texCoord);

//    float waterStart = 12.0 / 15.0;
//    float waterEnd = 1;
//
//    if (v_texCoord.x >= waterStart && v_texCoord.x <= waterEnd) {
//        texColor.a = 0.7;
//    }

    color = texColor;
}