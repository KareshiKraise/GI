#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

class uniform_buffer {
public:
	unsigned int index, size;
	uniform_buffer(unsigned int sz);
	void bind();
	void unbind();
	void upload_data(const void* data);
	void set_binding_point(unsigned int b);
	
};
