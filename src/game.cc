#include "game.hh"

#include "res/models/cube.hh"

using namespace std;
using namespace glm;

void Camera::update() {
	front = vec3(
		cos(rot.x) * sin(rot.y),
		sin(rot.x),
		cos(rot.x) * cos(rot.y)
	);
	right = vec3(
		sin(rot.y - PI/2),
		0,
		cos(rot.y - PI/2)
	);
	up = cross(right, front);
}

void Sky::load() {
	// -- load model
	vector<vec3> vertices, normals;
	vector<vec2> texCoords;
	vector<u32> indices;
	loadAssImp("res/models/skydome.obj", indices, vertices, normals, texCoords);
	indexCount = indices.size();
	//fprintf(stderr, "sky: did build arrays (v=%lu, n=%lu, t=%lu, i=%d)\n", vertices.size(), normals.size(), texCoords.size(), indexCount);

	// -- load texture
	tex = loadTexture("res/images/skydome.png");

	// -- load shader
	Shader vs("res/shaders/sky_v.glsl", GL_VERTEX_SHADER);
	Shader fs("res/shaders/sky_f.glsl", GL_FRAGMENT_SHADER);
	shader = ShaderProgram({ &vs, &fs });

	// -- send attributes
	glGenVertexArrays(1, &vao);
	glGenBuffers(4, vbo);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof_vec(vertices), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(shader.a("a_Vertex"), 3, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(shader.a("a_Vertex"));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof_vec(normals), normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(shader.a("a_Normal"), 3, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(shader.a("a_Normal"));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof_vec(texCoords), texCoords.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(shader.a("a_TexCoord"), 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(shader.a("a_TexCoord"));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof_vec(indices), indices.data(), GL_STATIC_DRAW);
}

void Sky::draw(mat4 P, mat4 V, vec3 origin) {
	glUseProgram(shader.id);
	glUniformMatrix4fv(shader.u("P"), 1, false, value_ptr(P));
	glUniformMatrix4fv(shader.u("V"), 1, false, value_ptr(V));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(shader.u("u_Texture"), 0);

	glBindVertexArray(vao);
	glDepthMask(false);

	mat4 M(1.0);
	M = translate(M, origin + vec3(0, -2000, 0));
	M = scale(M, vec3(10000));
	glUniformMatrix4fv(shader.u("M"), 1, false, value_ptr(M));
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

	glDepthMask(true);
}

void ColorCube::load() {
	Shader vs("res/shaders/colored_v.glsl", GL_VERTEX_SHADER);
	Shader fs("res/shaders/colored_f.glsl", GL_FRAGMENT_SHADER);
	shader = ShaderProgram({ &vs, &fs });

	glGenVertexArrays(1, &vao);
	glGenBuffers(3, vbo);

	// predefine cube
	glBindVertexArray(vao);

	// coorinate buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Cube::vertex), Cube::vertex, GL_STATIC_DRAW);
	glVertexAttribPointer(shader.a("vertex"), 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(shader.a("vertex"));

	// color buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Cube::color), Cube::color, GL_STATIC_DRAW);
	glVertexAttribPointer(shader.a("color"), 4, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(shader.a("color"));

	// index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Cube::index), Cube::index, GL_STATIC_DRAW);
	indexCount = Cube::indexCount;
}

void ColorCube::draw(mat4 P, mat4 V) {
	glUseProgram(shader.id);

	mat4 M = mat4(1.0);

	glUniformMatrix4fv(shader.u("P"), 1, false, value_ptr(P));
	glUniformMatrix4fv(shader.u("V"), 1, false, value_ptr(V));
	glUniformMatrix4fv(shader.u("M"), 1, false, value_ptr(M));

	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
}

