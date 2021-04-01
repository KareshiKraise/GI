#pragma once

#include <GL/glew.h>
#include "GL_CALL.h"

struct vao {
	GLuint vao_id;
	GLuint vbo_id;
	GLuint ibo_id;
	int varray;

	vao();
	vao(void* data, int size);
	void init(void* data, int size);	
	void set_element_buffer(unsigned int* data, int size);
	void set_vertexarrays(int components, int size, int offset, int type);	
	void bind();
	void unbind();	
	void from_ssbo(GLuint ssbo);
};
