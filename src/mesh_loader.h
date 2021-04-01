#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "model.h"
#include "texture.h"
//#define SHOW_MSG 1

unsigned int TextureFromFile(const char* path, const std::string& directory, aiTextureType type, bool gamma);

enum class model_type {
	TEXTURED = 1,
	NO_TEXTURE = 2,
};

struct mesh_loader
{
	mesh_loader();
	mesh_loader(const char* path, model_type type = model_type::TEXTURED);

	void lazy_init(const char* path, model_type type = model_type::TEXTURED);

	void Draw(program shader);
	void bb_center();
	glm::vec3 bb_mid;
	glm::vec3 bb_size;

	Model get_mesh() {
		std::cout << "models vector has " << models.size() << " models" << std::endl;
		return models.back();
	}
	std::vector<c_scene> cornel;
	std::vector<Model> models;
	std::vector<Texture> textures_loaded;
	std::string  directory;
	bool gammaCorrection;
	model_type m_type;
	glm::vec3 max;
	glm::vec3 min;

	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	Model processModel(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};