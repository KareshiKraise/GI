#include "SSBO.h"

SSBO::SSBO(unsigned int siz, unsigned int type_size)
{
	this->size = siz;
	this->type_size = type_size;
	GLCall(glGenBuffers(1, &this->ssbo));
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo));
	GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, siz* type_size, NULL, GL_DYNAMIC_COPY));
	unbind();
}

SSBO::SSBO(unsigned int siz, unsigned int type_size , void* data)
{
	this->size = siz;	
	this->type_size = type_size;
	GLCall(glGenBuffers(1, &this->ssbo));
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo));
	GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, siz*type_size, data, GL_DYNAMIC_COPY));
	unbind();
}

void SSBO::bindBase(unsigned int index)
{
	GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, this->ssbo));
}

void SSBO::bind()
{
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo));
}

void SSBO::unbind()
{
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
}
