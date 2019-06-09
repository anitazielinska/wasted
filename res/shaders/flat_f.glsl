#version 330 core

in vec2 TexCoord;

uniform sampler2D DiffuseMap0;

out vec4 color;

void main() {
	color = texture(DiffuseMap0, TexCoord);
}
