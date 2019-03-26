#include "techniques.h"

void cluster_vpls(Shader& cluster_program, const framebuffer& rsm_buffer, int num_VALs, int num_VPLs, bool start_frame, uniform_buffer& kvals)
{
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	cluster_program.use();

	kvals.bind();
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.pos));
	cluster_program.setInt("rsm_position", 0);

	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.normal));
	cluster_program.setInt("rsm_normal", 1);

	GLCall(glActiveTexture(GL_TEXTURE2));
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.albedo));
	cluster_program.setInt("rsm_flux", 2);

	cluster_program.setInt("num_VALs", num_VALs);
	cluster_program.setInt("num_VPLs", num_VPLs);
	cluster_program.setBool("start_frame", start_frame);
	kvals.set_binding_point(8);

	GLCall(glDispatchCompute(1,1,1));

	kvals.unbind();
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
}

void update_clusters(Shader& update_cluster_program, int num_VALs, int num_VPLs)
{
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));

	update_cluster_program.use();
	update_cluster_program.setInt("num_VALs", num_VALs);
	update_cluster_program.setInt("num_VPLs", num_VPLs);	

	GLCall(glDispatchCompute(1, 1, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
}

void shadow_pass(shadow_data& data, framebuffer& depth_buffer, mesh_loader& mesh, glm::mat4& model) {

	glm::mat4 lightspacemat = data.proj * data.view;

	/*----- SHADOW MAP PASS-----*/
	//glCullFace(GL_FRONT);
	data.shader->use();
	data.shader->setMat4("PV", lightspacemat);
	data.shader->setMat4("M", model);
	glViewport(0, 0, data.s_w, data.s_h);
	depth_buffer.bind();
	glClear(GL_DEPTH_BUFFER_BIT);
	mesh.Draw(*data.shader);
	depth_buffer.unbind();

	/*--------------------------*/

	glCullFace(GL_BACK);
}

std::vector<glm::vec2> gen_uniform_samples(unsigned int s, float min, float max) {
	std::uniform_real_distribution<float> randomFloats(min, max);
	//std::normal_distribution<float> randomFloats(min, max);
	std::default_random_engine gen;
	std::vector<glm::vec2> samples(s);

	for (int i = 0; i < s; i++) {
		glm::vec2 val{ 0,0 };
		val.x = randomFloats(gen);
		val.y = randomFloats(gen);
		samples[i] = val;
	}
	return samples;
}

//Draw instanced spheres
void draw_spheres(framebuffer& gbuffer, framebuffer& rsm_buffer, scene& sphere_scene, Model& sphere, Shader& sphereShader, GLuint sphereVAO, glm::vec3 mid, glm::vec2 coord) {
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
	sphereShader.setMat4("V", sphere_scene.camera->GetViewMatrix());

	sphereShader.setVec2("Coords", coord);

	glm::mat4 mod = glm::scale(glm::mat4(1.0), glm::vec3(1.0, 1.0, 1.0));
	glm::translate(mod, -mid);

	sphereShader.setMat4("M", mod);

	GLCall(glBindVertexArray(sphereVAO));
	GLCall(glDrawElementsInstanced(GL_TRIANGLES, sphere.get_indices().size(), GL_UNSIGNED_INT, 0, VPL_SAMPLES));
}

void do_SSVP(Shader& SSVP, shader_storage_buffer& lightSSBO, GLuint samplesTBO, const framebuffer& cubemap, glm::vec4 refPos, int NumLights, int MaxLights, unsigned int factor)
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
	SSVP.setInt("vpl_factor", factor);
	SSVP.setInt("MaxVPLs", MaxLights);
	SSVP.setVec4("refPos", refPos);

	GLCall(glBindImageTexture(0, samplesTBO, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F));
	//GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, lightSSBO));
	lightSSBO.bindBase(5);
	
	
	GLCall(glDispatchCompute(1, 1, 1));

	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

}

void fill_lightSSBO(const framebuffer& buffer, const glm::mat4& view, Shader& ssbo_program, GLuint tbo, shader_storage_buffer& ssbo, int samples, float radius)
{
	//std::cout << "ssbo is empty, will generate vpl buffer" << std::endl;
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

	ssbo_program.setFloat("vpl_radius", radius);
	
	//ssbo_program.setMat4("V", view);

	GLCall(glBindImageTexture(0, tbo, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F));	
	//GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo));
	ssbo.bindBase(5);

	GLCall(glDispatchCompute(samples, 1, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	ssbo.unbind();
}

void do_tiled_shading(Shader& tiled_shading, framebuffer& gbuffer, framebuffer& rsm_buffer, GLuint draw_tex, glm::mat4& invProj, scene& sponza, shadow_data& shadowmap, int numVPL, int numVAL, shader_storage_buffer& lightSSBO, float Wid, float Hei, float near, float far, const std::vector<glm::mat4>& pView, const std::vector<framebuffer>& pfbo, bool see_bounce) {

	tiled_shading.use();

	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.pos));
	tiled_shading.setInt("positionBuffer", 0);

	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.normal));
	tiled_shading.setInt("normalBuffer", 1);

	GLCall(glActiveTexture(GL_TEXTURE2));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.albedo));
	tiled_shading.setInt("albedoBuffer", 2);

	GLCall(glActiveTexture(GL_TEXTURE3));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map));
	tiled_shading.setInt("depthBuffer", 3);

	GLCall(glActiveTexture(GL_TEXTURE4));
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer.depth_map));
	tiled_shading.setInt("shadow_map", 4);

	GLCall(glActiveTexture(GL_TEXTURE5));
	GLCall(glBindTexture(GL_TEXTURE_2D, pfbo[0].depth_map));
	tiled_shading.setInt("pShadowMap1", 5);

	GLCall(glActiveTexture(GL_TEXTURE6));
	GLCall(glBindTexture(GL_TEXTURE_2D, pfbo[1].depth_map));
	tiled_shading.setInt("pShadowMap2", 6);

	GLCall(glActiveTexture(GL_TEXTURE7));
	GLCall(glBindTexture(GL_TEXTURE_2D, pfbo[2].depth_map));
	tiled_shading.setInt("pShadowMap3", 7);

	GLCall(glActiveTexture(GL_TEXTURE8));
	GLCall(glBindTexture(GL_TEXTURE_2D, pfbo[3].depth_map));
	tiled_shading.setInt("pShadowMap4", 8);

	GLCall(glBindImageTexture(0, draw_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F));
	tiled_shading.setMat4("invProj", invProj);
	tiled_shading.setMat4("ViewMat", sponza.camera->GetViewMatrix());
	tiled_shading.setMat4("lightSpaceMat", shadowmap.light_space_mat);
	tiled_shading.setMat4("parabolicMat1", pView[0]);
	tiled_shading.setMat4("parabolicMat2", pView[1]);
	tiled_shading.setMat4("parabolicMat3", pView[2]);
	tiled_shading.setMat4("parabolicMat4", pView[3]);
	tiled_shading.setVec3("sun_Dir", shadowmap.lightDir);
	tiled_shading.setVec3("eyePos", sponza.camera->Position);
	tiled_shading.setInt("num_vpls", numVPL);
	tiled_shading.setInt("num_vals", numVAL);
	tiled_shading.setFloat("near", near);
	tiled_shading.setFloat("far", far);	
	tiled_shading.setBool("double_bounce", see_bounce);
	GLCall(glDispatchCompute(ceil(Wid / 16), ceil(Hei / 16), 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
}

void generate_tile_frustum(Shader& frustum_program, shader_storage_buffer& buffer, float Wid, float Hei, glm::mat4& invProj)
{
	frustum_program.use();
	buffer.bindBase(3);
	frustum_program.setMat4("invProj", invProj);
	frustum_program.setFloat("Wid", Wid);
	frustum_program.setFloat("Hei", Hei);
	GLCall(glDispatchCompute(ceil(Wid / 16), ceil(Hei / 16), 1));
	buffer.unbind();
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
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

void generate_rsm_sampling_pattern(std::vector<glm::vec2>& p, RSM_parameters& rsm_param) {

	/*-------- RSM SHADING PARAMETERS ----------*/
	std::uniform_real_distribution<float> random(0.0f, 1.0f);
	std::default_random_engine gen;

	//normalized random numbers
	float sig1; //first random
	float sig2; //second random

	p.resize(rsm_param.SAMPLING_SIZE);//precalculated sampling pattern

	for (int i = 0; i < rsm_param.SAMPLING_SIZE; ++i) {
		//normalized random numbers
		sig1 = random(gen); //first random
		sig2 = random(gen); //second random
		p[i].x = sig1 * sin(PI_TWO *sig2);
		p[i].y = sig1 * cos(PI_TWO *sig2);
	}
}

void blur_pass(quad& screen, unsigned int source_id, Shader& blur_program, framebuffer& blur_buffer) {

	blur_buffer.bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	blur_program.use();

	GLCall(glActiveTexture(GL_TEXTURE0));
	blur_program.setInt("source", 0);
	GLCall(glBindTexture(GL_TEXTURE_2D, source_id));

	screen.renderQuad();
	blur_buffer.unbind();
}

void compute_vpl_propagation(Shader& ssvp, const std::vector<glm::mat4>& pView, const framebuffer& gbackbuffer, const std::vector<framebuffer>& paraboloidmaps,
	GLuint samplesTBO, float near, float far, int NUM_VPLS, int num_VALs, float vpl_radius)
{	
	ssvp.use();
	
	//vpl near and far planes
	//ssvp.setFloat("near", near);
	//ssvp.setFloat("far", far);
	//ssvp.setInt("num_vals", num_VALs);
	//
	//ssvp.setMat4("pMat1", pView[0]);
	//ssvp.setMat4("pMat2", pView[1]);
	//ssvp.setMat4("pMat3", pView[2]);
	//ssvp.setMat4("pMat4", pView[3]);
	
	GLCall(glBindImageTexture(0, samplesTBO, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F));

	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbackbuffer.pos));
	ssvp.setInt("prsm_pos", 0);
	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbackbuffer.normal));
	ssvp.setInt("prsm_norm", 1);
	GLCall(glActiveTexture(GL_TEXTURE2));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbackbuffer.albedo));
	ssvp.setInt("prsm_flux", 2);

	ssvp.setFloat("vpl_radius", vpl_radius);

	//GLCall(glActiveTexture(GL_TEXTURE3));
	//GLCall(glBindTexture(GL_TEXTURE_2D, paraboloidmaps[0].depth_map));
	//ssvp.setInt("parabolic_map1", 3);
	//GLCall(glActiveTexture(GL_TEXTURE4));
	//GLCall(glBindTexture(GL_TEXTURE_2D, paraboloidmaps[1].depth_map));
	//ssvp.setInt("parabolic_map2", 4);
	//GLCall(glActiveTexture(GL_TEXTURE5));
	//GLCall(glBindTexture(GL_TEXTURE_2D, paraboloidmaps[2].depth_map));
	//ssvp.setInt("parabolic_map3", 5);
	//GLCall(glActiveTexture(GL_TEXTURE6));
	//GLCall(glBindTexture(GL_TEXTURE_2D, paraboloidmaps[3].depth_map));
	//ssvp.setInt("parabolic_map4", 6);

	GLCall(glDispatchCompute(1, 1, 1));

	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
}

void create_sphere_vao(GLuint& sphereVAO, GLuint& sphereVBO, GLuint& sphereIBO, GLuint& instanceVBO, Model& sphere, std::vector<glm::vec2>& samples) {
	GLCall(glGenVertexArrays(1, &sphereVAO));
	GLCall(glGenBuffers(1, &sphereVBO));
	GLCall(glGenBuffers(1, &sphereIBO));

	GLCall(glBindVertexArray(sphereVAO));
	int size = 0;
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, sphereVBO));
	size = sphere.get_verts().size() * sizeof(vertex);
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, sphere.get_verts().data(), GL_STATIC_DRAW));

	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIBO));
	size = sphere.get_indices().size() * sizeof(unsigned int);
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, sphere.get_indices().data(), GL_STATIC_DRAW));

	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0));

	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, uv)));

	GLCall(glEnableVertexAttribArray(2));
	GLCall(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, normal)));
	GLCall(glBindVertexArray(0));

	GLCall(glGenBuffers(1, &instanceVBO));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, instanceVBO));
	GLCall(glBufferData(GL_ARRAY_BUFFER, samples.size() * sizeof(glm::vec2), samples.data(), GL_STATIC_DRAW));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));

	GLCall(glBindVertexArray(sphereVAO));
	GLCall(glEnableVertexAttribArray(3));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, instanceVBO));
	GLCall(glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GLCall(glVertexAttribDivisor(3, 1));
	GLCall(glBindVertexArray(0));
}

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

//stencil 1
void do_stencil_pass(framebuffer& gbuffer, framebuffer& rsm_buffer,  scene& sponza, Shader& stencil_pass, GLuint sphereVAO,  glm::vec3& sphere_mid,  glm::vec2& dims, Model& sphere) {
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

//stencil 2
void do_stencil_sphere_draw(framebuffer& gbuffer, framebuffer& rsm_buffer, scene& sponza, Model& sphere, Shader& sphere_shader, GLuint sphereVAO, glm::vec3& sphere_mid, float Wid, float Hei )
{
	gbuffer.set_intermediate_pass();
	glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
	glDisable(GL_DEPTH_TEST);			
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	draw_spheres(gbuffer, rsm_buffer, sponza, sphere, sphere_shader, sphereVAO, sphere_mid, glm::vec2(Wid, Hei));
	glCullFace(GL_BACK);
	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);			
}

//stencil 3
void do_directional_pass(framebuffer& gbuffer, Shader& dLightProgram, shadow_data& shadowmap, scene& sponza, framebuffer& depthBuffer, quad& screen_quad)
{
	gbuffer.set_intermediate_pass();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	dLightProgram.use();
	dLightProgram.setVec3("lightDir", shadowmap.lightDir);
	dLightProgram.setVec3("eyePos", sponza.camera->Position);
	dLightProgram.setMat4("LightSpaceMat", shadowmap.light_space_mat);			
	dLightProgram.setMat4("V", sponza.camera->GetViewMatrix());
	//gbuffer pos
	GLCall(glActiveTexture(GL_TEXTURE0));
	dLightProgram.setInt("gposition", 0);
	glBindTexture(GL_TEXTURE_2D, gbuffer.pos);
	//gbuffer normal
	GLCall(glActiveTexture(GL_TEXTURE1));
	dLightProgram.setInt("gnormal", 1);
	glBindTexture(GL_TEXTURE_2D, gbuffer.normal);
	//gbuffer albedo + spec
	GLCall(glActiveTexture(GL_TEXTURE2));
	dLightProgram.setInt("galbedo", 2);
	glBindTexture(GL_TEXTURE_2D, gbuffer.albedo);
	//shadow map
	GLCall(glActiveTexture(GL_TEXTURE3));
	dLightProgram.setInt("shadow_map", 3);
	glBindTexture(GL_TEXTURE_2D, depthBuffer.depth_map);
	screen_quad.renderQuad();
	glDisable(GL_BLEND);
}


void rsm_pass(Shader& RSM_pass, framebuffer& rsm, shadow_data& light_data, scene& s) {
	/*---------RSM_Pass--------*/
	//glCullFace(GL_FRONT);
	RSM_pass.use();
	RSM_pass.setMat4("P", light_data.proj);
	RSM_pass.setMat4("V", light_data.view);
	RSM_pass.setMat4("M", s.model);
	RSM_pass.setFloat("lightColor", 0.65f);
	RSM_pass.setVec3("lightDir", light_data.lightDir);
	glViewport(0, 0, light_data.s_w, light_data.s_h);
	rsm.bind();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	s.mesh->Draw(RSM_pass);
	rsm.unbind();
	//glCullFace(GL_BACK);
}

void gbuffer_pass(framebuffer& gbuffer, Shader& gbuf_program, scene& s, glm::mat4& view) {

	gbuffer.bind();
	glViewport(0, 0, gbuffer.w_res, gbuffer.h_res);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	gbuf_program.use();
	gbuf_program.setMat4("M", s.model);
	gbuf_program.setMat4("V", view);
	gbuf_program.setMat4("P", s.proj);
	s.mesh->Draw(gbuf_program);
	gbuffer.unbind();
}

void deep_g_buffer_pass(Shader& dgb_program, framebuffer& dgbuffer, scene& s, float delta) {

	glViewport(0, 0, dgbuffer.w_res, dgbuffer.h_res);
	dgbuffer.bind();
	dgbuffer.copy_fb_data();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	s.view = s.camera->GetViewMatrix();
	dgb_program.use();

	dgb_program.setMat4("M", s.model);
	dgb_program.setMat4("V", s.view);
	dgb_program.setMat4("P", s.proj);

	dgb_program.setInt("LAYERS", dgbuffer.layers);
	dgb_program.setFloat("DELTA", delta);
	dgb_program.setFloat("near", s.n_val);
	dgb_program.setFloat("far", s.f_val);

	GLCall(glActiveTexture(GL_TEXTURE4));
	dgb_program.setInt("compare_buffer", 4);
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, dgbuffer.compare_depth));

	s.mesh->Draw(dgb_program);

	dgbuffer.unbind();

}

void deep_g_buffer_debug(Shader& debug_view, framebuffer& dgb, quad& screen, shadow_data&  light_data, scene& s, framebuffer& depth_buffer) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	debug_view.use();

	glActiveTexture(GL_TEXTURE0);
	debug_view.setInt("gposition", 0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, dgb.pos);

	glActiveTexture(GL_TEXTURE1);
	debug_view.setInt("gnormal", 1);
	glBindTexture(GL_TEXTURE_2D_ARRAY, dgb.normal);

	glActiveTexture(GL_TEXTURE2);
	debug_view.setInt("galbedo", 2);
	glBindTexture(GL_TEXTURE_2D_ARRAY, dgb.albedo);

	glActiveTexture(GL_TEXTURE3);
	debug_view.setInt("shadow_map", 3);
	glBindTexture(GL_TEXTURE_2D, depth_buffer.depth_map);


	debug_view.setVec3("lightDir", light_data.lightDir);
	debug_view.setVec3("lightColor", light_data.lightColor);
	debug_view.setVec3("eyePos", s.camera->Position);
	debug_view.setMat4("lightSpaceMat", light_data.light_space_mat);

	screen.renderQuad();
}

void do_parabolic_map(const glm::mat4& parabolicModelView, framebuffer& parabolic_sm, Shader& parabolic_map, float ism_w, float ism_h, scene& sponza)
{
	glViewport(0, 0, ism_w, ism_h);
	parabolic_map.use();
	parabolic_sm.bind();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	parabolic_map.setMat4("parabolicModelView", parabolicModelView);
	sponza.mesh->Draw(parabolic_map);
	parabolic_sm.unbind();
}

void do_parabolic_rsm(const glm::mat4& View, framebuffer& parabolic_sm, Shader& parabolic_map, float ism_w, float ism_h, scene& sponza, float n, float f)
{
	glViewport(0, 0, ism_w, ism_h);
	parabolic_map.use();
	parabolic_sm.bind();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	parabolic_map.setMat4("M", sponza.model);
	parabolic_map.setMat4("V", View);
	parabolic_map.setFloat("near", n);
	parabolic_map.setFloat("far", f);
	sponza.mesh->Draw(parabolic_map);
	parabolic_sm.unbind();
}