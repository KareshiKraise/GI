#include "techniques.h"


/*
void draw_stenciled_spheres(framebuffer& gbuffer, framebuffer& rsm_buffer, scene& sphere_scene, Model& sphere, Shader& sphereShader, GLuint sphereVAO, glm::vec3& mid, glm::vec2& coord) {
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	sphereShader.use();

	glActiveTexture(GL_TEXTURE0);
	sphereShader.setInt("rsmposition", 0);
	glBindTexture(GL_TEXTURE_2D, rsm_buffer.pos);

	glActiveTexture(GL_TEXTURE1);
	sphereShader.setInt("rsmnormal", 1);
	glBindTexture(GL_TEXTURE_2D, rsm_buffer.normal);

	glActiveTexture(GL_TEXTURE2);
	sphereShader.setInt("rsmflux", 2);
	glBindTexture(GL_TEXTURE_2D, rsm_buffer.albedo);

	glActiveTexture(GL_TEXTURE3);
	sphereShader.setInt("gposition", 3);
	glBindTexture(GL_TEXTURE_2D, gbuffer.pos);

	glActiveTexture(GL_TEXTURE4);
	sphereShader.setInt("gnormal", 4);
	glBindTexture(GL_TEXTURE_2D, gbuffer.normal);

	glActiveTexture(GL_TEXTURE5);
	sphereShader.setInt("galbedo", 5);
	glBindTexture(GL_TEXTURE_2D, gbuffer.albedo);

	sphereShader.setMat4("P", sphere_scene.proj);
	sphereShader.setMat4("V", sphere_scene.view);

	sphereShader.setVec2("Coords", coord);

	glm::mat4 mod = glm::scale(glm::mat4(1.0), glm::vec3(1.0, 1.0, 1.0));
	glm::translate(mod, -mid);

	sphereShader.setMat4("M", mod);

	GLCall(glBindVertexArray(sphereVAO));
	GLCall(glDrawElementsInstanced(GL_TRIANGLES, sphere.get_indices().size(), GL_UNSIGNED_INT, 0, VPL_SAMPLES));
}

void do_stencil_pass(framebuffer& gbuffer, framebuffer& rsm_buffer,  scene& sponza, Shader& stencil_pass, GLuint sphereVAO,  glm::vec3& sphere_mid,  glm::vec2& dims) {
	glEnable(GL_STENCIL_TEST);	
	gbuffer.set_stencil_pass();
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glClear(GL_STENCIL_BUFFER_BIT);
	glStencilFunc(GL_ALWAYS, 0, 0);			
	glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
	glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);	
	draw_stenciled_spheres(gbuffer, rsm_buffer, sponza, sphere, stencil_pass, sphereVAO, sphere_mid, dims);	
}
*/