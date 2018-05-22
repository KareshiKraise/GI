#include "framebuffer.h"

framebuffer::framebuffer() {

}

framebuffer::framebuffer(fbo_type type, unsigned int w = 1024, unsigned int h = 1024) {
	fbo = 0;
	GLCall(glGenFramebuffers(1, &fbo));

	if (type == fbo_type::SHADOW_MAP) {
		fb_type = type;
		
		w_res = w;
		h_res = h;

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

		float border_col[] = {1.0f, 1.0f, 1.0f, 1.0f};
		GLCall(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_col));

		bind();
		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map, 0));
		GLCall(glDrawBuffer(GL_NONE));
		GLCall(glReadBuffer(GL_NONE));
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cout << "framebuffer incomplete" << std::endl;
		}
		std::cout <<  "shadow map fbo complete" << std::endl;
		unbind();
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

