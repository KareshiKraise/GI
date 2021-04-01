#pragma once

#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "program.h"
#include "VAO.h"

//colored textureless vertex struct
struct c_vertex {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec3 col;
};

//textureless scene
struct c_scene {
	c_scene(std::vector<c_vertex> a, std::vector<unsigned int> b);
	void setupMesh();
	void Draw();
	std::vector<c_vertex> cv;
	std::vector<unsigned int> idx;

	GLuint vao;
	GLuint vbo;
	GLuint ibo;
};

std::ostream& operator<< (std::ostream& stream, const c_vertex& c);

struct vertex {
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 bitangent;
};

struct Texture {
	GLuint id;
	std::string type;
	std::string path;
};

struct Model
{
	friend class mesh_loader;

	Model(std::vector<vertex> verts,
		std::vector<unsigned int> inds,
		std::vector<Texture> texts);
	~Model();

	void Draw(program shader);

	std::vector<vertex>& get_verts() {
		return vertices;
	};
	std::vector<unsigned int>& get_indices() {
		return indices;
	};
	void setupMesh();

	//GLuint vao;
	//GLuint vbo;
	//GLuint ibo;

	vao vertexarray;

	std::vector<vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;
};
