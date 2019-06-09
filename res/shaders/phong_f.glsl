#version 330 core

in vec2 TexCoord;
in vec4 l;
in vec4 n;
in vec4 v;

uniform sampler2D DiffuseMap0;
uniform sampler2D NormalMap0;

out vec4 color;

void main(void) {
	//Material color for a diffused light
	vec4 kd = texture(DiffuseMap0, TexCoord);
	vec4 ld = vec4(1,1,1,1); //Color of the diffused light
	vec4 ks = vec4(1,1,1,1); //Material color for a reflected light
	vec4 ls = vec4(1,1,1,1); //Color of the reflected light

	vec4 ml = normalize(l);
	vec4 mn = normalize(n);
	vec4 mv = normalize(v);
	//Reflection direction vector in the eye space
	vec4 mr = reflect(-ml, mn);

	//cosine of the angle between n and l vectors
	float nl = clamp(dot(mn, ml),0,1); 

	//cosine of the angle between r and v vectors to the power of Phong exponent
	float rv = pow(clamp(dot(mr, mv), 0, 1), 25); 

	color = vec4(kd.rgb*ld.rgb*nl + ks.rgb*ls.rgb*rv, kd.a);
}
