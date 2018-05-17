#pragma once
#include <GL/glew.h>
#include "GL_CALL.h"

enum class fbo_type {
	SHADOW_MAP = 0x01,
};

class framebuffer {
public:
	unsigned int shadow_h, shadow_w;
	unsigned int fbo;
	unsigned int depth_map;
	fbo_type fb_type;
	framebuffer();
	framebuffer(fbo_type type, unsigned int s_w, unsigned int s_h);
	void bind();
	void unbind();
};

