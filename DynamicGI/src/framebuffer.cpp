#include "framebuffer.h"

framebuffer::framebuffer() {

}

framebuffer::framebuffer(fbo_type type, unsigned int w = 1024, unsigned int h = 1024) : fbo{0} {
	
	GLCall(glGenFramebuffers(1, &fbo));

	if (type == fbo_type::SHADOW_MAP) {
		fb_type = type;		
		w_res = w;
		h_res = h;
		gen_shadow_map_fb();		
	}
	else if (type == fbo_type::G_BUFFER) {
		fb_type = type;
		w_res = w;
		h_res = h;
		gen_g_buffer();

	}
	else if (type == fbo_type::DEEP_G_BUFFER) {
		//
	}
	else if (type == fbo_type::RSM) {
		//
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

void framebuffer::gen_shadow_map_fb() {
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

	bind();
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map, 0));
	GLCall(glDrawBuffer(GL_NONE));
	GLCall(glReadBuffer(GL_NONE));
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "framebuffer incomplete" << std::endl;
	}
	std::cout << "shadow map fbo complete" << std::endl;
	unbind();
}

void framebuffer::gen_g_buffer() {
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

	gen_depth_buffer();

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "framebuffer incomplete" << std::endl;
	}
	std::cout << "g buffer fbo complete" << std::endl;
	unbind();

}

//renderbuffer
void framebuffer::gen_depth_buffer() {
	GLCall(glGenRenderbuffers(1, &depth_map));
	glBindRenderbuffer(GL_RENDERBUFFER, depth_map);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w_res, h_res);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_map);
	
}