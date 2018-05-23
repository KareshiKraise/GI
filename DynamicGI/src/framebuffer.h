#pragma once
#include <GL/glew.h>
#include "GL_CALL.h"

//will have other types such as G-BUFFER and DEEP-G-BUFFER in the future
enum class fbo_type {
	SHADOW_MAP = 0x01,
	RSM = 0X02,
	G_BUFFER = 0X03,
	DEEP_G_BUFFER = 0X04
};

class framebuffer {
public:
	unsigned int h_res, w_res;
	unsigned int fbo;
	unsigned int depth_map;
	unsigned int pos, normal, albedo;
	fbo_type fb_type;
	framebuffer();
	
	framebuffer(fbo_type type, unsigned int w, unsigned int h);
	void bind();
	void unbind();
private:
	void gen_shadow_map_fb(); 
	void gen_g_buffer();
	void gen_depth_buffer(); //renderbuffer for standard gbuffers
};

