#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "GL_CALL.h"

struct SSBO{
	SSBO(unsigned int size, unsigned int type_size);
	SSBO(unsigned int size, unsigned int type_size,  void* data);
	void bindBase(unsigned int index);
	void bind();
	void unbind();
	GLuint ssbo;
	unsigned int size;
	unsigned int type_size;	
};