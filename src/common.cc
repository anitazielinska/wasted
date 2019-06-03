#include "common.hh"

#include <cstdio>
#include <sstream>
#include "glmmodel/glmmodel.h"
#include "lodepng/lodepng.h"

using namespace std;

f64 clip(f64 value, f64 min, f64 max) {
	value = value < min ? min : value;
	value = value > max ? max : value;
	return value;
}

f64 rad(f64 deg) {
	return deg * PI / 180.0;
}

u32 loadTexture(const char *path) {
	u32 id;
	u8 *data;
	u32 width, height;
	lodepng_decode32_file(&data, &width, &height, path);
	if (!data) fprintf(stderr, "error: could not read image '%s'\n", path);

	/*
	u8 *temp = new u8[width * 4];
	for(u32 i = 0; i < height / 2; i++) {
		// copy row into temp array
		memcpy(temp, &data[i * width * 4], (width * 4));
		// copy other side of array into this row
		memcpy(&data[i * width * 4], &data[(height - i - 1) * width * 4], (width * 4));
		// copy temp into other side of array
		memcpy(&data[(height - i - 1) * width * 4], temp, (width * 4));
	}
	*/

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	return id;
}

bool readFile(const char *path, string &data) {
	ifstream file(path, ios::in);
	if (!file.is_open()) {
		fprintf(stderr, "error: couldn't open file '%s'\n", path);
		return false;
	}
	stringstream stream;
	stream << file.rdbuf();
	file.close();
	data = stream.str();
	return true;
}

void fatalError(const char *message) {
	fprintf(stderr, "fatal error: %s\n", message);
	exit(1);
}

i32 checkShaderStatus(u32 id) {
	i32 logLength = 0;
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0) {
		vector<char> message(logLength + 1);
		glGetShaderInfoLog(id, logLength, NULL, &message[0]);
		fprintf(stderr, "%s\n", message.data());
	}

	i32 result = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	return result;
}

Shader::Shader(const char *path, i32 type) {
	fprintf(stderr, "info: compiling shader '%s'\n", path);

	string code;
	if (!readFile(path, code)) return;
	char const *code_ptr = code.c_str();

	id = glCreateShader(type);
	glShaderSource(id, 1, &code_ptr, NULL);
	glCompileShader(id);
	checkShaderStatus(id);
}

ShaderProgram::ShaderProgram(vector<Shader*> shaders) {
	fprintf(stderr, "info: linking shader program\n");
	id = glCreateProgram();

	for (Shader *shader : shaders) {
		glAttachShader(id, shader->id);
	}

	glLinkProgram(id);
	checkShaderStatus(id);

	for (Shader *shader : shaders) {
		glDetachShader(id, shader->id);
	}
}

