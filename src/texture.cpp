#include "texture.h"

GLuint texture2D::gen_tex(unsigned int w, unsigned int h, int dtype, int nchannels, int format, void* data, int filter, int clamp)
{
	w=w;
	h=h;
	dtype=dtype;
	nchannels=nchannels;
	format=format;

	GLuint id;
	
	GLCall(glGenTextures(1, &id));
	GLCall(glBindTexture(GL_TEXTURE_2D, id));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, nchannels, dtype, data));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp));

	return id;
}

GLuint texture2D::gen_tex(unsigned int w, unsigned int h, int dtype, int nchannels, int format, int filter, int clamp)
{
	w = w;
	h = h;
	dtype = dtype;
	nchannels = nchannels;
	format = format;	

	GLuint id;

	GLCall(glGenTextures(1, &id));
	GLCall(glBindTexture(GL_TEXTURE_2D, id));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, nchannels, dtype, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp));

	return id;
}

GLuint texture2D::gen_tbo(unsigned int size, void* data) {
	GLuint tbo_buff_id=0;
	GLuint tbo_id=0;

	GLCall(glGenBuffers(1, &tbo_buff_id));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, tbo_buff_id));
	GLCall(glBufferData(GL_TEXTURE_BUFFER, size, data, GL_DYNAMIC_READ));
	GLCall(glGenTextures(1, &tbo_id));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, tbo_id));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, tbo_buff_id));

	return tbo_id;
}