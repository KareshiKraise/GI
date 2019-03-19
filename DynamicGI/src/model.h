#pragma once

#include <iostream>
#include <vector>

#include <GL/glew.h>

#include <glm/glm.hpp>

#include "shader.h"


struct c_vertex {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec3 col;
};

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
};


struct Texture {
	GLuint id;
	std::string type;
	std::string path;
};

class Model
{
public:
	friend class mesh_loader;

	Model(std::vector<vertex> verts,
		std::vector<unsigned int> inds,
		std::vector<Texture> texts);
	~Model();

	void Draw(Shader shader);
	
	std::vector<vertex>& get_verts() {
		return vertices;
	};


	std::vector<unsigned int>& get_indices() {
		return indices;
	};

private:

	void setupMesh();

	GLuint vao;
	GLuint vbo;
	GLuint ibo;

	std::vector<vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

};
