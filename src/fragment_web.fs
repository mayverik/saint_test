#version 300 es
precision mediump float;

uniform sampler2D uTexture;

in vec4 vColor;
in vec2 vTexCoord;

out vec4 oColor;

void main() {
    oColor = texture(uTexture, vTexCoord) * vColor;
}