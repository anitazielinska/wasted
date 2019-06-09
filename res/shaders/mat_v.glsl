#version 330 core

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 texCoord;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

uniform vec3 lightPos;
uniform vec3 viewPos;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 TangentLightPos;
out vec3 TangentViewPos;
out vec3 TangentFragPos;

void main(void) {
	FragPos = vec3(M * vec4(vertex, 1.0));   
	TexCoords = texCoord;

	mat3 normalMatrix = transpose(inverse(mat3(M)));
	vec3 T = normalize(normalMatrix * tangent);
	vec3 N = normalize(normalMatrix * normal);
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);

	mat3 TBN = transpose(mat3(T, B, N));    
	TangentLightPos = TBN * lightPos;
	TangentViewPos  = TBN * viewPos;
	TangentFragPos  = TBN * FragPos;

	gl_Position = P * V * M * vec4(vertex, 1.0);
}

