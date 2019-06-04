#pragma once

#include "util.hh"

u32 loadTexture(string path);

u32 loadCubemap(vector<string> faces);

bool loadAssImp(const char *path,  vector<u32> &indices,
		vector<vec3> &vertices, vector<vec3> &normals, vector<vec2> &uvs);

struct Mesh {
	vector<vec3> vertices, normals;
	vector<vec2> uvs;
	vector<u32> indices;
};

struct Shader {
	u32 id = 0;

	Shader(string file_path, i32 type);
	~Shader() { glDeleteShader(id); }
};

struct ShaderProgram {
	u32 id;

	ShaderProgram(u32 id = 0): id(id) {};
	ShaderProgram(vector<Shader*> shaders);
	~ShaderProgram() { glDeleteProgram(id); };
	ShaderProgram& operator=(ShaderProgram &&other) { std::swap(id, other.id); return *this; };
	ShaderProgram& operator=(const ShaderProgram& other) = delete;

	u32 u(const char *name) { return glGetUniformLocation(id, name); };
	u32 a(const char *name) { return glGetAttribLocation(id, name); };
};

