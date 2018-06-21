#pragma once
#include <GL/glew.h>
#include "GL_CALL.h"

//will have other types such as G-BUFFER and DEEP-G-BUFFER in the future
enum class fbo_type {
	SHADOW_MAP = 0x01,
	RSM = 0X02,
	G_BUFFER = 0X03,
	DEEP_G_BUFFER = 0X04,
	RADIOSITY_PARAM = 0x05,
	PRE_SHADE_EXPERIMENTAL = 0X06 
};

class framebuffer {
public:
	unsigned int layers;
	unsigned int h_res, w_res;
	unsigned int fbo;
	unsigned int depth_map, compare_depth;
	unsigned int pos, normal, albedo; //obs the albedo variable can also be used as the flux
	fbo_type fb_type;

	framebuffer();	
	framebuffer(fbo_type type, unsigned int w, unsigned int h, unsigned int l);

	void copy_fb_data();

	void bind();
	void unbind();
private:
	bool gen_radiosity();
	bool gen_shadow_map_fb(); 
	bool gen_g_buffer();
	bool gen_rsm();
	void gen_depth_renderbuffer(); //renderbuffer for standard gbuffers
	bool gen_dgbuffer();
	bool gen_indirect_buffer();
};

