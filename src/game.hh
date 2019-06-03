#pragma once

#include "common.hh"
#include "res/models/cube.hh"

#include "glmmodel/glmmodel.h"

using namespace glm;

struct Camera {
	// rot.x: pitch, rot.y: yaw, rot.z: roll (roll not implemented)
	vec3 pos, rot, front, right, up, world;

	Camera(vec3 pos, vec3 rot, vec3 front = vec3(0, 0, -1), vec3 up = vec3(0, 1, 0)):
			pos(pos), front(front), up(up), rot(rot) {
		world = vec3(0, 1, 0);
		update();
	}

	mat4 lookAt() {
		return glm::lookAt(pos, pos + front, up);
	}

	void update() {
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

	void offsetFront(f32 delta) { pos += front * delta; }
	void offsetRight(f32 delta) { pos += right * delta; }
	void offsetUp(f32 delta)    { pos += world * delta; }
	void offsetPitch(f32 delta) { rot.x += delta; update(); }
	void offsetYaw(f32 delta)   { rot.y += delta; update(); }
};

struct Sky {
	ShaderProgram shader;
	u32 vao, vbo[3], tex;
	i32 vertexCount;

	void load() {
		const char *obj_path = "res/models/sky.obj";
		const char *png_path = "res/images/sky.png";

		GLMmodel *model = glmReadOBJ((char*)obj_path); 
		if (!model) fprintf(stderr, "error: model not found '%s'\n", obj_path);
		glmScale(model, 1);
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(3, vbo);
		glmBuildVBO(model, &vertexCount, &vao, vbo);

		tex = loadTexture(png_path);

		Shader vs("res/shaders/sky_v.glsl", GL_VERTEX_SHADER);
		Shader fs("res/shaders/sky_f.glsl", GL_FRAGMENT_SHADER);
		shader = ShaderProgram({ &vs, &fs });
	}

	void draw(mat4 P, mat4 V, vec3 origin) {
		glUseProgram(shader.id);
		glUniformMatrix4fv(shader.u("P"), 1, false, value_ptr(P));
		glUniformMatrix4fv(shader.u("V"), 1, false, value_ptr(V));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);
		glBindVertexArray(vao);
		//glDisable(GL_BLEND);
		glDepthMask(false);

		mat4 M(1.0);
		M = translate(M, origin);
		M = scale(M, vec3(100000));
		glUniformMatrix4fv(shader.u("M"), 1, false, value_ptr(M));
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);

		M = rotate(M, PI, vec3(0, 0, 1));
		glUniformMatrix4fv(shader.u("M"), 1, false, value_ptr(M));
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);

		glDepthMask(true);
	}
};

struct ColorCube {
	ShaderProgram shader;
	u32 vao, vbo[3];

	void load() {
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
	}

	void draw(mat4 P, mat4 V) {
		glUseProgram(shader.id);

		mat4 M = mat4(1.0);

		glUniformMatrix4fv(shader.u("P"), 1, false, value_ptr(P));
		glUniformMatrix4fv(shader.u("V"), 1, false, value_ptr(V));
		glUniformMatrix4fv(shader.u("M"), 1, false, value_ptr(M));

		glBindVertexArray(vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
		glDrawElements(GL_TRIANGLES, Cube::indexCount, GL_UNSIGNED_INT, 0);
	}
};

