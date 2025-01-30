#version 330 core

uniform sampler2D sceneTex;
uniform float fadeAmount;

in Vertex {
    vec2 texCoord;
} IN;

out vec4 fragColor;

void main(void) {
    vec4 colour = texture(sceneTex, IN.texCoord);
    fragColor = mix(colour, vec4(0,0,0,1), fadeAmount);
}