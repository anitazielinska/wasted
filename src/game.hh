#pragma once

#include "util.hh"
#include "engine.hh"

struct Camera {
	// rot.x: pitch, rot.y: yaw, rot.z: roll (roll not implemented)
	vec3 pos, rot, front, up, right, world;

	Camera(vec3 pos, vec3 rot): pos(pos), rot(rot) {
		world = vec3(0, 1, 0);
		front = vec3(0, 0, -1);
		up = vec3(0, 1, 0);
		update();
	}

	mat4 lookAt() { return glm::lookAt(pos, pos + front, up); }
	void update();

	void offsetFront(f32 delta) { pos += front * delta; }
	void offsetRight(f32 delta) { pos += right * delta; }
	void offsetUp(f32 delta)    { pos += world * delta; }
	void offsetPitch(f32 delta) { rot.x += delta; update(); }
	void offsetYaw(f32 delta)   { rot.y += delta; update(); }
};

struct Sky {
	ShaderProgram shader;
	u32 vao, vbo[4], tex;
	i32 vertexCount;
	i32 indexCount;

	void load();
	void draw(mat4 P, mat4 V, vec3 origin);
};

struct ColorCube {
	ShaderProgram shader;
	u32 vao, vbo[3];
	i32 indexCount;

	void load();
	void draw(mat4 P, mat4 V);
};

