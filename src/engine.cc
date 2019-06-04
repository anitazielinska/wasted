#include "engine.hh"

#include <lodepng/lodepng.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace std;
using namespace glm;

void loadAssImpMesh(
	const aiMesh *mesh,
	vector<u32> &indices,
	vector<vec3> &vertices,
	vector<vec3> &normals,
	vector<vec2> &uvs
){
	const u32 vertexCount = mesh->mNumVertices;
	vertices.resize(vertexCount);
	normals.resize(vertexCount);
	uvs.resize(vertexCount);

	for (u32 i = 0; i < mesh->mNumVertices; i++) {
		aiVector3D v = mesh->mVertices[i];
		vertices[i] = vec3(v.x, v.y, v.z);

		aiVector3D n = mesh->mNormals[i];
		normals[i] = vec3(n.x, n.y, n.z);

		// Assume 1 set of UV coords; AssImp supports 8 UV sets.
		aiVector3D t = mesh->mTextureCoords[0][i];
		uvs[i] = vec2(t.x, t.y);
	}

	for (u32 i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (u32 j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}
}

bool loadAssImp(
	const char * path, 
	vector<u32> &indices,
	vector<vec3> &vertices,
	vector<vec3> &normals,
	vector<vec2> &uvs
){

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_FlipUVs);

	if (!scene) {
		fprintf(stderr, "error: assimp error '%s'\n", importer.GetErrorString());
		return false;
	}

	// Assume one mesh
	const aiMesh *mesh = scene->mMeshes[0];
	loadAssImpMesh(mesh, indices, vertices, normals, uvs);
	
	return true;
}


struct PNG {
	u32 width, height;
	u8 *data;
};

PNG loadPNG(string path) {
	const char *path_str = path.c_str();
	u32 width, height;
	u8 *data;
	lodepng_decode32_file(&data, &width, &height, path_str);
	if (!data) fprintf(stderr, "error: could not read image '%s'\n", path_str);
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
	delete[] temp;
	*/
	PNG png = { width, height, data };
	return png;
}

u32 loadTexture(string path) {
	u32 id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	PNG png = loadPNG(path);
	if (!png.data) return 0;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
			png.width, png.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, png.data);
	delete[] png.data;

	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	return id;
}

u32 loadCubemap(vector<string> faces) {
	u32 id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);

	for (u32 i = 0; i < faces.size(); i++) {
		string path = faces[i];
		PNG png = loadPNG(path);
		if (!png.data) return 0;
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
				png.width, png.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, png.data);
		delete[] png.data;
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return id;
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

Shader::Shader(string path, i32 type) {
	fprintf(stderr, "info: compiling shader '%s'\n", path.c_str());

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

