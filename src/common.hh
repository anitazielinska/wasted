#pragma once

#include <cstdint>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define PI 3.14159f

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

typedef GLFWwindow Window;

bool readFile(const char *path, std::string &data);

u32 loadTexture(const char *path);

void fatalError(const char *message);

f64 clip(f64 value, f64 min, f64 max);
f64 rad(f64 deg);
f64 deg(f64 rad);

struct Shader {
	u32 id = 0;

	Shader(const char *file_path, i32 type);
	~Shader() { glDeleteShader(id); }
};

struct ShaderProgram {
	u32 id;

	ShaderProgram(u32 id = 0): id(id) {};
	ShaderProgram(std::vector<Shader*> shaders);

	~ShaderProgram() {
		if (id) {
			glDeleteProgram(id);
			fprintf(stderr, "info: deleting shader program %u\n", id);
		}
	};

	ShaderProgram& operator=(ShaderProgram &&other) {
		std::swap(id, other.id);
		return *this;
	};

	ShaderProgram& operator=(const ShaderProgram& other) = delete;

	u32 u(const char *name) { return glGetUniformLocation(id, name); };
	u32 a(const char *name) { return glGetAttribLocation(id, name); };
};

