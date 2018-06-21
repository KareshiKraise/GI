#include "framebuffer.h"

framebuffer::framebuffer() {

}

framebuffer::framebuffer(fbo_type type, unsigned int w = 1024, unsigned int h = 1024, unsigned int l = 2) : fbo{ 0 } {
	
	GLCall(glGenFramebuffers(1, &fbo));

	if (type == fbo_type::SHADOW_MAP) {
		fb_type = type;		
		w_res = w;
		h_res = h;
		layers = l;
		if (gen_shadow_map_fb()) {				
			std::cout << "shadow map fbo complete" << std::endl;
		}
	}
	else if (type == fbo_type::G_BUFFER) {
		fb_type = type;
		w_res = w;
		h_res = h;
		layers = l;
		if(gen_g_buffer())
			std::cout << "gbuffer fbo complete" << std::endl;

	}
	else if (type == fbo_type::DEEP_G_BUFFER) {
		fb_type = type;
		w_res = w;
		h_res = h;
		layers = l;
		if(gen_dgbuffer())
			std::cout << "deep gbuffer fbo complete" << std::endl;
	}
	else if (type == fbo_type::RSM) {
		fb_type = type;
		w_res = w;
		h_res = h;
		layers = l;
		if(gen_rsm())
			std::cout << " rsm fbo complete" << std::endl;
	}

	else if (type == fbo_type::RADIOSITY_PARAM) {
		fb_type = type;
		w_res = w;
		h_res = h;
		layers = l;
		if (gen_radiosity())
			std::cout << " radiosity fbo complete" << std::endl;
	}

	else if (type == fbo_type::PRE_SHADE_EXPERIMENTAL) {
		fb_type = type;
		w_res = w;
		h_res = h;
		layers = l;
		if (gen_indirect_buffer())
			std::cout << " indirect lighting fbo complete" << std::endl;
	}
}

void framebuffer::bind() {
	if (fbo) {
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
	}
}

void framebuffer::unbind() {
	if (fbo) {
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER,0));
	}
}

bool framebuffer::gen_shadow_map_fb() {
	bind();
	GLCall(glGenTextures(1, &depth_map));
	GLCall(glBindTexture(GL_TEXTURE_2D, depth_map));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, w_res, h_res, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

	//compare mode for hardware supported shadowmapping, opengl 4.3+, to be used with textureProj() inside GLSL
	//GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,GL_COMPARE_REF_TO_TEXTURE));
	//GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL));

	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	float border_col[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLCall(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_col));

	
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map, 0));
	GLCall(glDrawBuffer(GL_NONE));
	GLCall(glReadBuffer(GL_NONE));
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "depth map framebuffer incomplete" << std::endl;
		return false;
	}
	
	unbind();
	return true;
}

bool framebuffer::gen_g_buffer() {
	bind();
	
	GLCall(glGenTextures(1, &pos));
	GLCall(glBindTexture(GL_TEXTURE_2D, pos));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w_res, h_res, 0, GL_RGB, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pos, 0));
	
	GLCall(glGenTextures(1, &normal));
	GLCall(glBindTexture(GL_TEXTURE_2D, normal));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w_res, h_res, 0, GL_RGB, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normal, 0));

	GLCall(glGenTextures(1, &albedo));
	GLCall(glBindTexture(GL_TEXTURE_2D, albedo));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w_res, h_res, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, albedo, 0));

	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	gen_depth_renderbuffer();

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "gbuffer framebuffer incomplete" << std::endl;
		return false;
	}
	
	unbind();
	return true;

}
//renderbuffer
void framebuffer::gen_depth_renderbuffer() {
	GLCall(glGenRenderbuffers(1, &depth_map));
	glBindRenderbuffer(GL_RENDERBUFFER, depth_map);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w_res, h_res);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_map);
	
}

bool framebuffer::gen_rsm() {
	bind();

	//position attachment
	GLCall(glGenTextures(1, &pos));
	GLCall(glBindTexture(GL_TEXTURE_2D, pos));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w_res, h_res, 0, GL_RGB, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pos, 0));
	

	//normal attachment
	GLCall(glGenTextures(1, &normal));
	GLCall(glBindTexture(GL_TEXTURE_2D, normal));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w_res, h_res, 0, GL_RGB, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normal, 0));
	

	//flux atachment
	GLCall(glGenTextures(1, &albedo));
	GLCall(glBindTexture(GL_TEXTURE_2D, albedo));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w_res, h_res, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, albedo, 0));
	
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	//gen_shadow_map_fb();
	gen_depth_renderbuffer();

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "RSM framebuffer incomplete" << std::endl;
		return false;
	}
	
	unbind();
	return true;
}

bool framebuffer::gen_dgbuffer() {
	
	bind();
	
	GLCall(glGenTextures(1, &pos));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, pos));
	GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB16F, w_res, h_res, layers, 0, GL_RGB, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pos, 0));

	float border_col[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLCall(glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border_col));

	GLCall(glGenTextures(1, &normal));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, normal));
	GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB16F, w_res, h_res, layers, 0, GL_RGB, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, normal, 0));

	
	GLCall(glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border_col));

	GLCall(glGenTextures(1, &albedo));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, albedo));
	GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, w_res, h_res, layers, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, albedo , 0));

	
	GLCall(glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border_col));

	GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	GLCall(glGenTextures(1, &depth_map));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, depth_map));
	GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, w_res, h_res, layers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_map, 0));

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "deep g buffer framebuffer incomplete" << std::endl;
		return false;
	}

	unbind();

	GLCall(glGenTextures(1, &compare_depth));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, compare_depth));
	GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, w_res, h_res, layers - 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	return true;

}

void framebuffer::copy_fb_data() {

	GLCall(glCopyImageSubData(depth_map, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, compare_depth, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, w_res, h_res, layers - 1));

}

bool framebuffer::gen_radiosity() {
	bind();
	

	//radiosity
	GLCall(glGenTextures(1, &albedo));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, albedo));
	GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, w_res, h_res, layers, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, albedo, 0));

	float border_col[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLCall(glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border_col));

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "radiosity framebuffer incomplete" << std::endl;
		return false;
	}

	unbind();
	return true;

}

bool framebuffer::gen_indirect_buffer() {
	bind();

	GLCall(glGenTextures(1, &albedo));
	GLCall(glBindTexture(GL_TEXTURE_2D, albedo));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w_res, h_res, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedo, 0));

	float border_col[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLCall(glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border_col));


	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "indirect lighting framebuffer incomplete" << std::endl;
		return false;
	}
	unbind();
	return true;
}
