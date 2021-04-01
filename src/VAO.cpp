#include "VAO.h"

vao::vao() : vao_id{ 0 }, vbo_id{ 0 }, ibo_id{ 0 }, varray{ 0 } {}

vao::vao(void* data, int size) {
	GLCall(glGenVertexArrays(1, &vao_id));
	GLCall(glGenBuffers(1, &vbo_id));
	GLCall(glGenBuffers(1, &ibo_id));

	GLCall(glBindVertexArray(vao_id));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo_id));
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));

	GLCall(glBindVertexArray(0));
	varray = 0;
}

void vao::init(void* data, int size) {
	GLCall(glGenVertexArrays(1, &vao_id));
	GLCall(glGenBuffers(1, &vbo_id));
	GLCall(glGenBuffers(1, &ibo_id));

	GLCall(glBindVertexArray(vao_id));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo_id));
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));

	GLCall(glBindVertexArray(0));
	varray = 0;
}

void vao::set_element_buffer(unsigned int* data, int size)
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
	//GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void vao::bind()
{
	GLCall(glBindVertexArray(vao_id));
}

void vao::unbind()
{
	GLCall(glBindVertexArray(0));
}

void vao::set_vertexarrays(int components, int size, int offset, int type)
{	
	GLCall(glEnableVertexAttribArray(varray));
	GLCall(glVertexAttribPointer(varray, components, type, GL_FALSE, size, (void*)offset));	
	varray++;	
}

void vao::from_ssbo(GLuint ssbo) {
	GLCall(glGenVertexArrays(1, &vao_id));
	GLCall(glBindVertexArray(vao_id));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, ssbo));
}