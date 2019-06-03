#version 330 core

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

in vec3 a_Vertex;
in vec3 a_Normal;
in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main() {
	v_TexCoord = a_TexCoord;
	gl_Position = P * V * M * vec4(a_Vertex, 1.0);
}
