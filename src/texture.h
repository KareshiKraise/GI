#pragma once

#include <GL/glew.h>
#include "GL_CALL.h"

namespace texture2D {
	//GLuint id;
	//int w;
	//int h;
	//int dtype;
	//int nchannels;
	//int format;	

	GLuint gen_tex(unsigned int w, unsigned int h, int dtype, int nchannels, int format, void* data, int filter, int clamp = GL_CLAMP_TO_BORDER);
	GLuint gen_tex(unsigned int w, unsigned int h, int dtype, int nchannels, int format, int filter, int clamp = GL_CLAMP_TO_BORDER);

	GLuint gen_tbo(unsigned int size, void* data);
};
//w corresponds to w dimension
//h corresponds to h dimension
//dtype corresponds to data type (GL_FLOAT, GL_UNSIGNED_BYTE)
//nchannels corresponds to num channels (GL_RGBA, GL_RED ...)
//format corresponds do data format (GL_RGBA16F, GL_RGBA8)
//void* data corresponds to texture data
//clamp corresponds to clamp mode