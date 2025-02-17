#version 330 core

uniform mat4 uMatrix;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;

out vec4 vColor;
out vec2 vTexCoord;

void main(){
    gl_Position = uMatrix * vec4(aPos, 1);
    vColor = aColor;
    vTexCoord = aTexCoord;
}