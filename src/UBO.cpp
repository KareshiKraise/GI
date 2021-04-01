#pragma once

#include "UBO.h"
#include "GL_CALL.h"

UBO::UBO(unsigned int sz) {
	size = sz;
	GLCall(glGenBuffers(1, &(this->index)));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, (this->index)));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));

}

// binds ubo, uploads data and then unbind the current ubo
void UBO::upload_data(const void* data) {
	bind();
	//int val;
	//glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_SIZE, &val);
	//std::cout << "bufer size is " << val << std::endl;
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data));
	unbind();
}

void UBO::bind() {
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, (this->index)));
}

void UBO::unbind() {
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void UBO::set_binding_point(unsigned int b) {
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, b, (this->index)));
}

