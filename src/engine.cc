#include "engine.hh"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace std;
using namespace glm;

// -----------------------------------------------

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

// -----------------------------------------------

const vector<TextureType> textureTypes = {
	TextureType::diffuse,
	TextureType::specular,
	TextureType::height,
	TextureType::normal
};

void Model::readMesh(aiMesh *mesh, Mesh &x) {
	u32 vertexCount = mesh->mNumVertices, faceCount = mesh->mNumFaces;
	x.material = mesh->mMaterialIndex;
	x.model = this;

	x.vertices.resize(vertexCount);
	for (u32 i = 0; i < vertexCount; i++) {
		aiVector3D v = mesh->mVertices[i];
		x.vertices[i] = vec3(v.x, v.y, v.z);
	}

	for (u32 i = 0; i < faceCount; i++) {
		aiFace face = mesh->mFaces[i];
		for (u32 j = 0; j < face.mNumIndices; j++) {
			x.indices.push_back(face.mIndices[j]);
		}
	}

	if (mesh->HasNormals()) {
		x.normals.resize(vertexCount);
		for (u32 i = 0; i < vertexCount; i++) {
			aiVector3D n = mesh->mNormals[i];
			x.normals[i] = vec3(n.x, n.y, n.z);
		}
	}

	if (mesh->HasTangentsAndBitangents()) {
		x.tangents.resize(vertexCount);
		x.bitangents.resize(vertexCount);
		for (u32 i = 0; i < vertexCount; i++) {
			aiVector3D t = mesh->mTangents[i];
			aiVector3D b = mesh->mBitangents[i];
			x.tangents[i] = vec3(t.x, t.y, t.z);
			x.bitangents[i] = vec3(b.x, b.y, b.z);
		}
	}

	// Assume 1 set of UV coords; AssImp supports 8 UV sets.
	if (mesh->HasTextureCoords(0)) {
		x.texCoords.resize(vertexCount);
		for (u32 i = 0; i < vertexCount; i++) {
			aiVector3D t = mesh->mTextureCoords[0][i];
			x.texCoords[i] = vec2(t.x, t.y);
		}
	}
}

void Model::readMaterial(aiMaterial *mat, Material &x) {
	aiColor3D c(0, 0, 0);
	aiString s;

	mat->Get(AI_MATKEY_COLOR_DIFFUSE, c);
	x.diffuse = vec3(c.r, c.g, c.b);
	mat->Get(AI_MATKEY_COLOR_SPECULAR, c);
	x.specular = vec3(c.r, c.g, c.b);
	mat->Get(AI_MATKEY_COLOR_AMBIENT, c);
	x.ambient = vec3(c.r, c.g, c.b);
	mat->Get(AI_MATKEY_COLOR_EMISSIVE, c);
	x.emissive = vec3(c.r, c.g, c.b);
	mat->Get(AI_MATKEY_COLOR_TRANSPARENT, c);
	x.transparent = vec3(c.r, c.g, c.b);

	mat->Get(AI_MATKEY_SHININESS, x.shininess);
	mat->Get(AI_MATKEY_SHININESS_STRENGTH, x.specularScale);
	mat->Get(AI_MATKEY_OPACITY, x.opacity);
	mat->Get(AI_MATKEY_TWOSIDED, x.backfaceCulling);
	mat->Get(AI_MATKEY_NAME, s);
	x.name = s.data;

	string directory = path.substr(0, path.find_last_of('/'));
	for (TextureType type : textureTypes) {
		aiTextureType aiType = static_cast<aiTextureType>(type);
		u32 count = mat->GetTextureCount(aiType);

		for (u32 i = 0; i < count; i++) {
			aiString name;
			mat->GetTexture(aiType, i, &name);
			string path = directory + "/" + name.data;

			// TODO: do not load duplicate textures
			Texture tex;
			tex.type = type;
			tex.path = path;
			textures.push_back(tex);
			x.textures.push_back(textures.size() - 1);

			dprintf("-- material %s texture %s\n", x.name.c_str(), tex.path.c_str());
		}
	}
}

void Model::boundingBox(aiMesh *mesh, vec3 &minCoords, vec3 &maxCoords) {
    u32 verticesCount = mesh->mNumVertices;
    for (u32 i = 0; i < verticesCount; i++) {
        if ( mesh->mVertices[i].x < minCoords.x ) minCoords.x = mesh->mVertices[i].x;
        if ( mesh->mVertices[i].y < minCoords.y ) minCoords.y = mesh->mVertices[i].y;
        if ( mesh->mVertices[i].z < minCoords.z ) minCoords.z = mesh->mVertices[i].z;
        if ( mesh->mVertices[i].x > maxCoords.x ) maxCoords.x = mesh->mVertices[i].x;
        if ( mesh->mVertices[i].y > maxCoords.y ) maxCoords.y = mesh->mVertices[i].y;
        if ( mesh->mVertices[i].z > maxCoords.z ) maxCoords.z = mesh->mVertices[i].z;
    }
}

void Model::read() {
	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs
			| aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
		eprintf("error: assimp error '%s'\n", importer.GetErrorString());
		return;
	}

	u32 materialCount = scene->mNumMaterials, meshCount = scene->mNumMeshes, textureCount = scene->mNumTextures;
	dprintf("info: reading model '%s' (%u meshes, %u materials, %u textures)\n",
			path.c_str(), meshCount, materialCount, textureCount);

	materials.resize(materialCount);
	for (u32 i = 0; i < materialCount; i++) {
		readMaterial(scene->mMaterials[i], materials[i]);
	}

    minCoords = vec3(scene->mMeshes[0]->mVertices[0].x, scene->mMeshes[0]->mVertices[0].y, scene->mMeshes[0]->mVertices[0].z);
    maxCoords = vec3(scene->mMeshes[0]->mVertices[0].x, scene->mMeshes[0]->mVertices[0].y, scene->mMeshes[0]->mVertices[0].z);
	meshes.resize(meshCount);
	for (u32 i = 0; i < meshCount; i++) {
		readMesh(scene->mMeshes[i], meshes[i]);
		boundingBox(scene->mMeshes[i], minCoords, maxCoords);
	}
    size = vec3(maxCoords.x - minCoords.x, maxCoords.y - minCoords.y, maxCoords.z - minCoords.z);
    center = vec3((minCoords.x + maxCoords.x)/2, (minCoords.y + maxCoords.y)/2, (minCoords.z + maxCoords.z)/2);
}

void Model::load() {
	read();
	for (Texture &x : textures) x.load();
	for (Mesh &x : meshes) x.load();
}

void Model::unload() {
	for (Texture &x : textures) x.unload();
	for (Mesh &x : meshes) x.unload();
}

void Model::draw(Program &shader) {
	for (Mesh &x : meshes) x.draw(shader);
}

void printModel(Model &model) {
	dprintf("------------\n");

	dprintf("model '%s':\n", model.path.c_str());

	dprintf("  textures:\n");
	for (u32 i = 0; i < model.textures.size(); i++) {
		Texture &tex = model.textures[i];
		dprintf("  - '%s'\n", tex.path.c_str());
	}

	dprintf("  materials:\n");
	for (u32 i = 0; i < model.materials.size(); i++) {
		Material &mat = model.materials[i];
		dprintf("  - '%s':\n", mat.name.c_str());
		dprintf("      diffuse:  (%.1f, %.1f, %.1f)\n", mat.diffuse.x, mat.diffuse.y, mat.diffuse.z);
		dprintf("      specular: (%.1f, %.1f, %.1f)\n", mat.specular.x, mat.specular.y, mat.specular.z);
		dprintf("      ambient:  (%.1f, %.1f, %.1f)\n", mat.ambient.x, mat.ambient.y, mat.ambient.z);
		dprintf("      emissive: (%.1f, %.1f, %.1f)\n", mat.emissive.x, mat.emissive.y, mat.emissive.z);
		dprintf("      transp:   (%.1f, %.1f, %.1f)\n", mat.transparent.x, mat.transparent.y, mat.transparent.z);
		for (u32 j = 0; j < mat.textures.size(); j++) {
			Texture &tex = model.textures[mat.textures[j]];
			dprintf("      texture: %s\n", tex.path.c_str());
		}
	}

	dprintf("  meshes:\n");
	for (u32 i = 0; i < model.meshes.size(); i++) {
		Mesh &mesh = model.meshes[i];
		Material &mat = model.materials[mesh.material];
		dprintf("  - %u vao=%u:\n", i, mesh.vao);
		dprintf("      material: '%s'\n", mat.name.c_str());
		dprintf("      indices:    %lu\n", mesh.indices.size());
		dprintf("      vertices:   %lu\n", mesh.vertices.size());
		dprintf("      normals:    %lu\n", mesh.normals.size());
		dprintf("      tangents:   %lu\n", mesh.tangents.size());
		dprintf("      bitangents: %lu\n", mesh.bitangents.size());
	}
}

// -----------------------------------------------

void Mesh::load() {
	glGenVertexArrays(1, &vao);
	glGenBuffers(6, vbo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof_vec(vertices), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof_vec(normals), normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof_vec(texCoords), texCoords.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, sizeof_vec(tangents), tangents.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(3, 3, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);
	glBufferData(GL_ARRAY_BUFFER, sizeof_vec(bitangents), bitangents.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(4, 3, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(4);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[5]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof_vec(indices), indices.data(), GL_STATIC_DRAW);
	indexCount = indices.size();

	glBindVertexArray(0);
}

void Mesh::unload() {
	glDeleteBuffers(1, &vao);
	glDeleteBuffers(6, vbo);
}

void Mesh::draw(Program &shader) {
	Material &mat = model->materials[material];

	u32 diffuseId = 0, normalId = 0, heightId = 0, specularId = 0;
	for (u32 i = 0; i < mat.textures.size(); i++) {
		Texture &tex = model->textures[mat.textures[i]];

		string var;
		switch (tex.type) {
			case TextureType::diffuse: var = "DiffuseMap" + to_string(diffuseId++); break;
			case TextureType::normal: var = "NormalMap" + to_string(normalId++); break;
			case TextureType::height: var = "HeightMap" + to_string(heightId++); break;
			case TextureType::specular: var = "SpecularMap" + to_string(specularId++); break;
		}

		//dprintf("info: -- %u tex(vao=%u, id=%u, name=%s) -> %s\n", i, vao, tex.id, tex.path.c_str(), var.c_str());
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, tex.id);
		glUniform1i(shader.u(var.c_str()), i);
	}

	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glActiveTexture(GL_TEXTURE0);
}

// -----------------------------------------------

void Texture::read() {
}

void Texture::load() {
	if (!ilLoadImage(path.c_str())) {
		eprintf("error: could not load image '%s'\n", path.c_str());
		return;
	}

	format = ilGetInteger(IL_IMAGE_FORMAT);
	height = ilGetInteger(IL_IMAGE_HEIGHT);
	width = ilGetInteger(IL_IMAGE_WIDTH);
	u8 *data = ilGetData();

	glGenTextures(1, &id);
	//dprintf("info: loading texture '%s' at id %u\n", path.c_str(), id);

	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void Texture::unload() {
	glDeleteTextures(1, &id);
}

// -----------------------------------------------

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

Shader::Shader(const string &path, i32 type) {
	string code;
	if (!readFile(path, code)) return;
	const char *code_ptr = code.c_str();

	eprintf("info: compiling shader '%s'\n", path.c_str());
	id = glCreateShader(type);
	glShaderSource(id, 1, &code_ptr, NULL);
	glCompileShader(id);
	checkShaderStatus(id);
}

void Program::load() {
	vector<Shader> shaders;
	shaders.push_back(Shader(vs, GL_VERTEX_SHADER));
	shaders.push_back(Shader(fs, GL_FRAGMENT_SHADER));
	if (!gs.empty()) shaders.push_back(Shader(gs, GL_GEOMETRY_SHADER));

	eprintf("info: linking shader program\n");
	id = glCreateProgram();

	for (Shader &shader : shaders) glAttachShader(id, shader.id);
	glLinkProgram(id);
	checkShaderStatus(id);
	for (Shader &shader : shaders) glDetachShader(id, shader.id);
}

void Program::unload() {
	glDeleteProgram(id);
}

void Program::reload() {
	unload();
	load();
}

