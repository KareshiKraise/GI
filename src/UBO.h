#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

struct UBO {
	unsigned int index, size;
	UBO(unsigned int sz);
	void bind();
	void unbind();
	void upload_data(const void* data);
	void set_binding_point(unsigned int b);
};
