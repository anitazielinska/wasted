#version 330 core

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

layout (location=0) in vec4 vertex;
layout (location=3) in vec4 color;

out vec4 i_c;

void main(void) {
    i_c = color;
    gl_Position = P*V*M*vertex;
}
