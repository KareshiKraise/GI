#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "GL_CALL.h""

class shader_storage_buffer {
public:
	shader_storage_buffer(unsigned int siz);	
	shader_storage_buffer(unsigned int siz, void* data);
	void bindBase(unsigned int index);	
	void bind();
	void unbind();
	GLuint ssbo;
	unsigned int size;

};