#version 330 core

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 ep;

out vec4 l;
out vec4 n;
out vec4 v;
out vec2 TexCoord;

void main(void) {
	vec4 vertex4 = vec4(vertex, 1.0);
	vec4 normal4 = vec4(normal, 1.0);
	vec4 lp = vec4(0, 10, 0, 1);

	// Vector "towards the light" in the eye space
	l = normalize(V*lp - V*M*vertex4);
	// Normal vector in the eye space
	n = normalize(V*M*normal4);
	// Vector "towards the viewer" in the eye space
	v = normalize(V*vec4(ep, 1) - V*M*vertex4); 

	TexCoord = texCoord;
	gl_Position = P*V*M*vertex4;
}
