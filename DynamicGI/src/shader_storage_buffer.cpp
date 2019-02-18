#include "shader_storage_buffer.h"

shader_storage_buffer::shader_storage_buffer(unsigned int siz)
{
	this->size = siz;
	GLCall(glGenBuffers(1, &this->ssbo));
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo));
	GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, size, NULL, GL_DYNAMIC_COPY));
}


void shader_storage_buffer::bindBase(unsigned int index)
{
	GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, this->ssbo));
}

void shader_storage_buffer::bind()
{
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo));
}


void shader_storage_buffer::unbind()
{
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
}
