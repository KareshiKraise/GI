#pragma once

#include "uniform_buffer.h"
#include "GL_CALL.h"

uniform_buffer::uniform_buffer(unsigned int sz) {
	size = sz;
	GLCall(glGenBuffers(1, &index));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, index));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	
}

// binds ubo, uploads data and then unbind the current ubo
void uniform_buffer::upload_data(const void* data) {
	
	bind();
	//int val;
	//glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_SIZE, &val);
	//std::cout << "bufer size is " << val << std::endl;
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data));
	unbind();
}

void uniform_buffer::bind() {
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, index));
}

void uniform_buffer::unbind() {
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void uniform_buffer::set_binding_point(unsigned int b) {
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, b, index));
}

