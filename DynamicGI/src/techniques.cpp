#include "techniques.h"

void do_SSVP(Shader& SSVP, GLuint lightSSBO, GLuint samplesTBO, const framebuffer& cubemap, glm::vec4 refPos, int NumLights, int MaxLights)
{
	SSVP.use();

	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.pos));
	SSVP.setInt("cubePos", 0);

	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.normal));
	SSVP.setInt("cubeNormal", 1);

	GLCall(glActiveTexture(GL_TEXTURE2));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.albedo));
	SSVP.setInt("cubeColor", 2);

	SSVP.setInt("NumLights", NumLights);
	SSVP.setInt("MaxVPLs", MaxLights);
	SSVP.setVec4("refPos", refPos);

	GLCall(glBindImageTexture(0, samplesTBO, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F));
	GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, lightSSBO));
	
	GLCall(glDispatchCompute(1, 1, 1));

	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

}

void fill_lightSSBO(const framebuffer& buffer, const glm::mat4& view, Shader& ssbo_program, GLuint tbo, GLuint ssbo)
{
	std::cout << "ssbo is empty, will generate vpl buffer" << std::endl;
	ssbo_program.use();
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, buffer.pos));
	ssbo_program.setInt("rsm_position", 0);

	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, buffer.albedo));
	ssbo_program.setInt("rsm_flux", 1);

	GLCall(glActiveTexture(GL_TEXTURE2));
	GLCall(glBindTexture(GL_TEXTURE_2D, buffer.normal));
	ssbo_program.setInt("rsm_normal", 2);

	ssbo_program.setMat4("V", view);

	GLCall(glBindImageTexture(0, tbo, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F));
	GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo));

	GLCall(glDispatchCompute(VPL_SAMPLES, 1, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}


glm::mat4 axis_from_rotation(float yaw, float pitch, glm::vec3 eye)
{	
	using namespace glm;
	
	mat4 viewMat = mat4(1.0f);
	viewMat = rotate(viewMat, radians(180.0f), vec3(0.0, 0.0, 1.0));
	viewMat = rotate(viewMat, radians(pitch),  vec3(1.0, 0.0, 0.0));
	viewMat = rotate(viewMat, radians(yaw), vec3(0.0, 1.0, 0.0));
	viewMat = translate(viewMat, -eye);
	return viewMat;
}

void draw_skybox(Shader& skybox_program, const framebuffer& cube_buf, glm::mat4 view, glm::mat4 proj, Cube& cube)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthFunc(GL_LEQUAL);
	skybox_program.use();
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, cube_buf.albedo));
	skybox_program.setInt("cubeColor", 0);
	skybox_program.setMat4("V", glm::mat4(glm::mat3(view)));
	skybox_program.setMat4("P", proj);
	cube.renderCube();
	glDepthFunc(GL_LESS);
}
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