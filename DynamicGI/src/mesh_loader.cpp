#include "mesh_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>

mesh_loader::mesh_loader() 
{

}

mesh_loader::mesh_loader(const char* path, model_type type)
{
	m_type = type;
	std::cout << "loading model at : " << path << std::endl;
	loadModel(path);	
	bb_center();
	std::cout << "model loaded" << std::endl;
}

void mesh_loader::Draw(Shader shader)
{
	if (m_type == model_type::TEXTURED)
	{
		for (auto& m : models)
		{
			m.Draw(shader);
		}
	}
	//else if(m_type == model_type::NO_TEXTURE) 
	//{
	//	for (auto& m : cornel)
	//	{
	//		m.Draw();
	//	}
	//}
}

//flags aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices
void mesh_loader::loadModel(std::string path)
{
#ifdef SHOW_MSG
	std::cout << "inside load model" << std::endl;
#endif
	Assimp::Importer importer; 
	
	const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices );
	scene = importer.ApplyPostProcessing(aiProcess_CalcTangentSpace);	
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "[ERROR:ASSIMP]: " << importer.GetErrorString() << std::endl;
		return;
	}

	directory = path.substr(0, path.find_last_of('/'));
	processNode(scene->mRootNode, scene);

#ifdef SHOW_MSG
	std::cout << "finished load model" << std::endl;
#endif
}

void mesh_loader::processNode(aiNode * node, const aiScene * scene)
{
#ifdef SHOW_MSG
	std::cout << "inside process node" << std::endl;
#endif
	for (int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		models.push_back(processModel(mesh, scene));
	}

	for (int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}

#ifdef SHOW_MSG
	std::cout << "finished process node" << std::endl;
#endif
}

Model mesh_loader::processModel(aiMesh * mesh, const aiScene * scene)
{
#ifdef SHOW_MSG
	std::cout << "inside process model" << std::endl;
#endif
	std::vector<vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;
	std::vector<c_vertex> col;

	for (int i = 0; i < mesh->mNumVertices; i++)
	{
		vertex vert;
		glm::vec3 vec;
		vec.x = mesh->mVertices[i].x;
		vec.y = mesh->mVertices[i].y;
		vec.z = mesh->mVertices[i].z;
		vert.pos = vec;

		vec.x = mesh->mNormals[i].x;
		vec.y = mesh->mNormals[i].y;
		vec.z = mesh->mNormals[i].z;
		vert.normal = vec;

		if (mesh->mTextureCoords[0])
		{
			glm::vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vert.uv = vec;
		}
		else {
			vert.uv = glm::vec2(0.0f, 0.0f);
			std::cout << "no uvs detected" << std::endl;
		}

		if (mesh->HasTangentsAndBitangents())
		{   
			//std::cout << "Has tangents" << std::endl;
			/*tangents and bitangents*/
			vec.x = mesh->mTangents[i].x;
			vec.x = mesh->mTangents[i].y;
			vec.x = mesh->mTangents[i].z;
			vert.tangent = vec;
			vec.x = mesh->mBitangents[i].x;
			vec.x = mesh->mBitangents[i].y;
			vec.x = mesh->mBitangents[i].z;
			vert.bitangent = vec;
			vertices.push_back(vert);
		}
		else
		{
			std::cout << "Model doesnt have tangents" << std::endl;
		}
		
		c_vertex v{ vert.pos, vert.normal, glm::vec3(0.0) };
		col.push_back(v);
	}

	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
	

	aiColor3D color(0.0, 0.0, 0.0);
	material->Get(AI_MATKEY_COLOR_DIFFUSE, color);

	for (auto& c : col)
	{
		c.col = glm::vec3(color.r, color.g, color.b);
	}
		
	std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

	std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
	textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

	std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
	textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());		

	std::vector<Texture> transparencyMaps = loadMaterialTextures(material, aiTextureType_OPACITY, "texture_alphamask");
	textures.insert(textures.end(), transparencyMaps.begin(), transparencyMaps.end());

	std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
	textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

#ifdef SHOW_MSG
	std::cout << "finished process model" << std::endl;
#endif
	cornel.push_back(c_scene(col, indices));

	return Model(vertices, indices, textures);
}

std::vector<Texture> mesh_loader::loadMaterialTextures(aiMaterial * mat, aiTextureType type, std::string typeName)
{
#ifdef SHOW_MSG
	std::cout << "inside loadmaterialtextures" << std::endl;
#endif
	std::vector<Texture> textures;
	for (int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		bool skip = false;
		for (int j = 0; j < textures_loaded.size(); j++)
		{
			if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
			{
				//std::cout << "texture: " << str.C_Str() << " already exists" << std::endl;
				textures.push_back(textures_loaded[j]);
				skip = true; break;
			}
		}
		if (!skip)
		{
			Texture texture;
			texture.id = TextureFromFile(str.C_Str(), this->directory, type, gammaCorrection);
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			textures_loaded.push_back(texture);
		}

	}

#ifdef SHOW_MSG
	std::cout << "finished load material textures" << std::endl;
#endif
	return textures;
}

void mesh_loader::bb_center()
{
	float minX = models[0].vertices[0].pos.x;
	float minY = models[0].vertices[0].pos.y;
	float minZ = models[0].vertices[0].pos.z;

	float maxX = models[0].vertices[0].pos.x;
	float maxY = models[0].vertices[0].pos.y;
	float maxZ = models[0].vertices[0].pos.z;

	for (auto& m : models)
	{

		for (int i = 0; i < m.vertices.size(); i++)
		{
			if (m.vertices[i].pos.x < minX)
			{
				minX = m.vertices[i].pos.x;
			}
			if (m.vertices[i].pos.x > maxX)
			{
				maxX = m.vertices[i].pos.x;
			}

			if (m.vertices[i].pos.y < minY)
			{
				minY = m.vertices[i].pos.y;
			}
			if (m.vertices[i].pos.y > maxY)
			{
				maxY = m.vertices[i].pos.y;
			}

			if (m.vertices[i].pos.z < minZ)
			{
				minZ = m.vertices[i].pos.z;
			}
			if (m.vertices[i].pos.z > maxZ)
			{
				maxZ = m.vertices[i].pos.z;
			}
		}
	}
	bb_mid = glm::vec3((minX + maxX) / 2.0f, (minY + maxY) / 2.0f, (minZ + maxZ) / 2.0f);

	max = glm::vec3(maxX, maxY, maxZ);
	min = glm::vec3(minX, minY, minZ);
}

unsigned int TextureFromFile(const char * path, const std::string& directory, aiTextureType type, bool gamma = false)
{
#ifdef SHOW_MSG
	std::cout << "inside TextureFromFile" << std::endl;
#endif
	std::string filename = std::string(path);

	//printf("%s\n",directory.c_str());
	//printf("%s\n",filename.c_str());
	filename = directory + '/' + filename;
	
	GLuint textureID;
	//std::cout << "generating texture " << directory << std::endl;
	GLCall(glGenTextures(1, &textureID));
	int width, height, nrComponents;
	//std::cout << "texture generated succesfully" << std::endl;	

	unsigned char * data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	//std::cout << "path of current texture " << filename << std::endl;
	//std::cout << "wid " << width << std::endl;
	//std::cout << "hei " << height << std::endl;
	//std::cout << "num components " << nrComponents << std::endl;
	
	if (data)
	{
		//std::cout << "data is valid" << std::endl;
		GLenum format;
		GLenum internal_format;
		if (nrComponents == 1)
		{
			format = GL_RED;
			internal_format = GL_RED;
		}
		else if (nrComponents == 2)
		{
			format = GL_RG;
			internal_format = GL_RG;
		}
		else if (nrComponents == 3)
		{
			format = GL_RGB;	
			internal_format = GL_RGB;			
			if (type == aiTextureType_DIFFUSE)
				internal_format = GL_SRGB;			
		}
		else if (nrComponents = 4)
		{
			format = GL_RGBA;	
			internal_format = GL_RGBA;
			if(type == aiTextureType_DIFFUSE)
				internal_format = GL_SRGB_ALPHA;
		}

		GLCall(glBindTexture(GL_TEXTURE_2D, textureID););
		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, data));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCall(glGenerateMipmap(GL_TEXTURE_2D));

		stbi_image_free(data);
	}
	else
	{
		//std::cout << "data is invalid" << std::endl;
		std::cout << "Texture failed to load at path: " << path << std::endl;
		std::cout << stbi_failure_reason() << std::endl;
		stbi_image_free(data);
	}
#ifdef SHOW_MSG
	std::cout << "finished TextureFromFile" << std::endl;
#endif

	return textureID;

}