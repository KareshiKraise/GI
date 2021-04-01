#include "render_passes.h"


void generate_gbuffer(framebuffer* fbo, program& shader, Transforms& mats, mesh_loader& model)
{
	fbo->bind();	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	shader.use();
	shader.setMat4("M", mats["M"]);
	shader.setMat4("P", mats["P"]);
	shader.setMat4("V", mats["V"]);
	model.Draw(shader);
	fbo->unbind();
}

void debug_blit(program& shader, quad& fsquad, framebuffer* layered, int cur_Layer)
{	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	shader.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, layered->depth_map);
	shader.setInt("texture1", 0);
	shader.setInt("layer", cur_Layer);
	fsquad.renderQuad();
}

void blit_to_screen(program& shader, framebuffer* gbuffer, GLuint radianceBuffer, float near, float far, quad& q) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	shader.use();
	GLCall(glActiveTexture(GL_TEXTURE0));
	glBindTexture(GL_TEXTURE_2D, radianceBuffer);
	shader.setInt("lightingBuffer", 0);
	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer->albedo));
	shader.setInt("materialTexture", 1);
	GLCall(glActiveTexture(GL_TEXTURE2));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer->depth_map));
	shader.setInt("depthImage", 2);
	shader.setFloat("near", near);
	shader.setFloat("far",  far);
	shader.setFloat("wid", gbuffer->h_res);
	shader.setFloat("hei", gbuffer->w_res);
	q.renderQuad();
}

void generate_rsm(framebuffer* fbo, program& shader, Transforms mats, mesh_loader& model, light_props& ldata) {
	shader.use();
	shader.setMat4("M", mats["M"]);
	shader.setMat4("V", mats["V_rsm"]);
	shader.setMat4("P", mats["P"]);
	shader.setVec3("lightColor", ldata.light_col);
	shader.setVec3("lightDir", ldata.light_dir);
	fbo->bind();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	model.Draw(shader);
	fbo->unbind();
}

void generate_shadowmap(framebuffer* fbo, program& shader, Transforms mats, mesh_loader& model, light_props& ldata)
{
	shader.use();
	shader.setMat4("M", mats["M"]);
	shader.setMat4("V", mats["V_shadowmap"]);
	shader.setMat4("P", mats["P"]);
	shader.setVec3("lightDir", ldata.light_dir);
	fbo->bind();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	model.Draw(shader);
	fbo->unbind();
}

void deferred_lighting(program& shader, quad& fsquad, Transforms& scene_transforms, Transforms& light_transforms, light_props& ldata, framebuffer* gbuffer, framebuffer* shadowmap, framebuffer* layered, int num_vpls_first_bounce, float pnear, float pfar, camera& c) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	shader.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gbuffer->pos);
	shader.setInt("gpos", 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gbuffer->normal);
	shader.setInt("gnormal", 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gbuffer->albedo);
	shader.setInt("gmaterial", 2);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gbuffer->depth_map);
	shader.setInt("gdepth", 3);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, shadowmap->depth_map);
	shader.setInt("shadowmap", 4);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D_ARRAY, layered->depth_map);
	shader.setInt("val_shadowmaps", 5);
	shader.setFloat("parabola_near",pnear);
	shader.setFloat("parabola_far",pfar);
	shader.setMat4("lightMat", light_transforms["P"]* light_transforms["V_shadowmap"]* light_transforms["M"]);
	shader.setMat4("M",light_transforms["M"]);
	shader.setVec3("lightDir", ldata.light_dir);
	shader.setVec3("lightColor", ldata.light_col);
	shader.setVec3("eye_pos", c.Position);
	shader.setInt("num_vpls", num_vpls_first_bounce);
	
	fsquad.renderQuad();
}

void generate_importance_map(program& shader, framebuffer* gbuffer, framebuffer* rsm, SSBO& brsm, SSBO& samples) {
	shader.use();
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer->pos));
	shader.setInt("gpos", 0);
	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer->normal));
	shader.setInt("gnormal", 1);
	GLCall(glActiveTexture(GL_TEXTURE2));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer->albedo));
	shader.setInt("galbedo", 2);
	GLCall(glActiveTexture(GL_TEXTURE3));
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm->pos));
	shader.setInt("rsm_pos", 3);
	GLCall(glActiveTexture(GL_TEXTURE4));
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm->normal));
	shader.setInt("rsm_normal", 4);
	GLCall(glActiveTexture(GL_TEXTURE5));
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm->albedo));
	shader.setInt("rsm_albedo", 5);
	shader.setFloat("RSM_W", rsm->w_res);
	shader.setFloat("RSM_H", rsm->h_res);
	shader.setInt("numViewSamples", 256);
	samples.bindBase(0);
	brsm.bindBase(1);
	GLCall(glDispatchCompute(rsm->w_res / 16, rsm->h_res / 16, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	brsm.unbind();
	samples.unbind();
}

void generate_cdf_cols(program& shader, SSBO& brsm, SSBO& cdf_cols, float rsm_w, float rsm_h) {
	shader.use();
	shader.setFloat("BRSM_W", rsm_w);
	shader.setFloat("BRSM_H", rsm_h);
	brsm.bindBase(0);
	cdf_cols.bindBase(1);
	int num_wkgrops = int(rsm_w) / 16;
	GLCall(glDispatchCompute(num_wkgrops, 1, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	cdf_cols.unbind();
	brsm.unbind();
};

void generate_cdf_rows(program& shader, SSBO& cdf_cols, SSBO& cdf_rows, float rsm_w, float rsm_h) {
	shader.use();
	shader.setFloat("BRSM_W", rsm_w);
	shader.setFloat("BRSM_H", rsm_h);
	cdf_cols.bindBase(0);
	cdf_rows.bindBase(1);
	GLCall(glDispatchCompute(1, 1, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	cdf_rows.unbind();
	cdf_cols.unbind();
};

void generate_vpls(program& shader, framebuffer* rsm, SSBO& ssbo_cdf_cols, SSBO& ssbo_cdf_rows, SSBO& ssbo_1st_vpls, int num_vpls_first_bounce, GLuint sample_cdf_tbo) {
	shader.use();
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm->pos));
	shader.setInt("rsm_pos", 0);
	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm->normal));
	shader.setInt("rsm_normal", 1);
	GLCall(glActiveTexture(GL_TEXTURE2));
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm->albedo));
	shader.setInt("rsm_albedo", 2);
	shader.setInt("RSM_W", rsm->w_res);
	shader.setInt("RSM_H", rsm->h_res);
	shader.setInt("num_vpls", num_vpls_first_bounce);
	ssbo_1st_vpls.bindBase(0);
	ssbo_cdf_cols.bindBase(1);
	ssbo_cdf_rows.bindBase(2);
	GLCall(glBindImageTexture(3, sample_cdf_tbo, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F));
	GLCall(glDispatchCompute((num_vpls_first_bounce / 16), 1, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	ssbo_cdf_rows.unbind();
	ssbo_cdf_cols.unbind();
	ssbo_1st_vpls.unbind();
}

void generate_vals(program& shader, SSBO& ssbo_1st_vpls, SSBO& ssbo_vals, int num_vpls_first_bounce, int num_clusters) {
	shader.use();
	shader.setInt("num_vpls", num_vpls_first_bounce);
	shader.setInt("num_clusters", num_clusters);
	ssbo_vals.bindBase(0);
	ssbo_1st_vpls.bindBase(1);
	GLCall(glDispatchCompute(num_clusters, 1, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	ssbo_1st_vpls.unbind();
	ssbo_vals.unbind();
}

void calc_distance_to_val(program& shader, int num_clusters, int num_vpls_first_bounce, bool first_pass, SSBO& ssbo_1st_vpls, SSBO& ssbo_vals, SSBO& vpl_to_vals, SSBO& vpl_count)
{
	ssbo_vals.bindBase(0);
	ssbo_1st_vpls.bindBase(1);
	vpl_to_vals.bindBase(2);
	vpl_count.bindBase(3);
	shader.use();
	shader.setInt("num_vals", num_clusters);
	shader.setInt("vpl_samples", num_vpls_first_bounce);
	shader.setInt("first_cluster_pass", first_pass);
	GLCall(glDispatchCompute(num_vpls_first_bounce/16, 1, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	vpl_count.unbind();
	vpl_to_vals.unbind();
	ssbo_1st_vpls.unbind();
	ssbo_vals.unbind();
}

void update_cluster_centers(program& shader, int num_vpls_first_bounce, int num_clusters, SSBO& ssbo_1st_vpls, SSBO& ssbo_vals, SSBO& vpl_to_vals, SSBO& vpl_count)
{
	ssbo_vals.bindBase(0);
	ssbo_1st_vpls.bindBase(1);
	vpl_to_vals.bindBase(2);
	vpl_count.bindBase(3);
	shader.use();
	shader.setInt("vpl_samples", num_vpls_first_bounce);
	GLCall(glDispatchCompute(num_clusters, 1, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	vpl_count.unbind();
	vpl_to_vals.unbind();
	ssbo_1st_vpls.unbind();
	ssbo_vals.unbind();
}

void render_cluster_shadow_map(program& shader, framebuffer* layered_fbo, float near, float far, mesh_loader& model, int num_clusters, Transforms& transforms)
{	
	layered_fbo->bind();
	shader.use();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	shader.setFloat("near", near);
	shader.setFloat("far", far);
	shader.setMat4("M", transforms["M"]);
	shader.setInt("num_vals", num_clusters);
	model.Draw(shader);
	layered_fbo->unbind();	
}

void propagate_vpls(program& shader, GLuint sample_texture, framebuffer* clusters_fbo, SSBO& sec_bounce_vpls, SSBO& sec_bounce_count, float radius, int num_clusters) {
	shader.use();
	sec_bounce_vpls.bindBase(0);
	sec_bounce_count.bindBase(1);
	shader.setFloat("vpl_radius", radius);
	GLCall(glBindImageTexture(0, sample_texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F));
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, clusters_fbo->pos));
	shader.setInt("prsm_pos", 0);
	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, clusters_fbo->normal));
	shader.setInt("prsm_norm", 1);
	GLCall(glActiveTexture(GL_TEXTURE2));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, clusters_fbo->albedo));
	shader.setInt("prsm_flux", 2);	
	GLCall(glDispatchCompute(num_clusters, 1, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	sec_bounce_count.unbind();
	sec_bounce_vpls.unbind();
}

void update_light_buffers(program& shader, SSBO& ssbo_1st_vpls, SSBO& ssbo_vals, SSBO& vpl_to_vals, SSBO& vpl_count, int num_vpls_first_bounce, int num_clusters) {
	ssbo_vals.bindBase(0);
	ssbo_1st_vpls.bindBase(1);
	vpl_to_vals.bindBase(2);
	vpl_count.bindBase(3);
	shader.use();
	shader.setInt("num_vpls", num_vpls_first_bounce);
	GLCall(glDispatchCompute(num_clusters, 1, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	ssbo_vals.unbind();
	ssbo_1st_vpls.unbind();
	vpl_to_vals.unbind();
	vpl_count.unbind();
}

void split_gbuffer(program& shader, framebuffer* gbuffer, framebuffer* interleaved_buffer, int n_rows, int n_cols, int Wid, int Hei)
{
	shader.use();
	//bind
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer->pos));
	shader.setInt("gposition", 0);

	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer->normal));
	shader.setInt("gnormal", 1);

	GLCall(glActiveTexture(GL_TEXTURE2));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer->albedo));
	shader.setInt("gcol", 2);

	GLCall(glActiveTexture(GL_TEXTURE3));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer->depth_map));
	shader.setInt("gdepth", 3);

	shader.setInt("Wid", Wid);
	shader.setInt("Hei", Hei);
	shader.setInt("n_rows", n_rows);
	shader.setInt("n_cols", n_cols);

	GLCall(glBindImageTexture(0, interleaved_buffer->pos, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F));
	GLCall(glBindImageTexture(1, interleaved_buffer->normal, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F));
	GLCall(glBindImageTexture(2, interleaved_buffer->albedo, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8));
	GLCall(glBindImageTexture(3, interleaved_buffer->depth_map, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F));

	GLCall(glDispatchCompute(Wid / 16, Hei / 16, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
}

void shade_interleaved_buffer(program& shader, GLuint drawtex, framebuffer* interleaved_gbuffer, framebuffer* layered_gbuffer, framebuffer* shadowmap,
	framebuffer* rsm, Transforms& light_transforms, int ncols, int nrows, int num_vpls_first_bounce,
	int num_clusters, int num_vpls_second_bounce, float near, float far, camera& c, light_props& ldata, bool see_bounce, int wid, int hei, bool direct_only, float spotlight_cutoff, bool is_spotlight) {

	shader.use();

	shader.setBool("see_bounce", see_bounce);
	shader.setBool("direct_only", direct_only);
	shader.setBool("is_spotlight", is_spotlight);
	shader.setInt("num_rows", nrows);
	shader.setInt("num_cols", ncols);
	shader.setInt("num_first_bounce", num_vpls_first_bounce);
	shader.setInt("num_second_bounce", num_vpls_second_bounce);
	shader.setInt("num_vals", num_clusters);
	shader.setInt("wid", wid);
	shader.setInt("hei", hei);
	shader.setFloat("parabola_near", near);
	shader.setFloat("parabola_far", far);

	shader.setFloat("spotlight_cutoff", spotlight_cutoff);

	shader.setVec3("sun_dir", ldata.light_dir);
	shader.setVec3("eye_pos", c.Position);
	shader.setVec3("lpos", ldata.light_pos);

	shader.setMat4("lightspacemat", light_transforms["P"] * light_transforms["V_shadowmap"] * light_transforms["M"]);
	shader.setMat4("M", light_transforms["M"]);

	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, interleaved_gbuffer->pos));
	shader.setInt("gposition", 0);

	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, interleaved_gbuffer->normal));
	shader.setInt("gnormal", 1);

	GLCall(glActiveTexture(GL_TEXTURE2));
	GLCall(glBindTexture(GL_TEXTURE_2D, interleaved_gbuffer->albedo));
	shader.setInt("galbedo", 2);

	GLCall(glActiveTexture(GL_TEXTURE3));
	GLCall(glBindTexture(GL_TEXTURE_2D, interleaved_gbuffer->depth_map));
	shader.setInt("gdepth", 3);

	GLCall(glActiveTexture(GL_TEXTURE4));
	GLCall(glBindTexture(GL_TEXTURE_2D, shadowmap->depth_map));
	shader.setInt("directionalLightMap", 4);

	GLCall(glActiveTexture(GL_TEXTURE5));
	GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, layered_gbuffer->depth_map));
	shader.setInt("val_shadowmaps", 5);

	GLCall(glBindImageTexture(0, drawtex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F));

	GLCall(glDispatchCompute(wid / 16, hei / 16, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
}

void join_buffers(program& shader, GLuint interleaved_tex, GLuint final_tex, int num_rows, int num_cols, int Wid, int Hei)
{
	shader.use();
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, interleaved_tex));
	shader.setInt("interleaved_image", 0);
	shader.setInt("Wid", Wid);
	shader.setInt("Hei", Hei);
	shader.setInt("n_rows", num_rows);
	shader.setInt("n_cols", num_cols);
	GLCall(glBindImageTexture(0, final_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F));
	GLCall(glDispatchCompute(Wid / 16, Hei / 16, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
}

void edge_detection(program& shader, framebuffer* gbuffer, GLuint edge_tex)
{
	shader.use();
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer->normal));
	shader.setInt("gnormal", 0);
	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, gbuffer->depth_map));
	shader.setInt("gdepth", 1);

	shader.setInt("Wid", gbuffer->w_res);
	shader.setInt("Hei", gbuffer->h_res);

	GLCall(glBindImageTexture(0, edge_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8));

	GLCall(glDispatchCompute(gbuffer->w_res / 16, gbuffer->h_res / 16, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
}

void horizontal_pass(program& xblur, GLuint edge_tex, GLuint in_tex, GLuint out_tex, quad& q, float Wid, float Hei)
{
	xblur.use();
	xblur.setFloat("Wid", Wid);
	xblur.setFloat("Hei", Hei);

	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, in_tex));
	xblur.setInt("lightingBuffer", 0);
	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, edge_tex));
	xblur.setInt("edgeBuffer", 1);
	GLCall(glBindImageTexture(0, out_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F));


	q.renderQuad();
}

void vertical_pass(program& yblur, GLuint edge_tex, GLuint in_tex, GLuint out_tex, quad& q, float Wid, float Hei)
{
	yblur.use();
	yblur.setFloat("Wid", Wid);
	yblur.setFloat("Hei", Hei);

	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, in_tex));
	yblur.setInt("lightingBuffer", 0);
	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, edge_tex));
	yblur.setInt("edgeBuffer", 1);
	GLCall(glBindImageTexture(0, out_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F));


	q.renderQuad();
}

void blur_pass(program& xblur, program& yblur, int num_blur_pass, GLuint edge_tex, GLuint in_tex, GLuint out_tex, quad& q, float Wid, float Hei)
{
	for (int i = 0; i < num_blur_pass; ++i)
	{
		horizontal_pass(xblur, edge_tex, in_tex, out_tex, q, Wid, Hei);
		vertical_pass(yblur, edge_tex, out_tex, in_tex, q, Wid, Hei);
	}
}

void sample_vpls_from_rsm(program& shader, framebuffer* rsm_buffer, SSBO& ssbo_1st_vpls, int num_vpls_first_bounce, GLuint samples_tbo) {
	ssbo_1st_vpls.bindBase(0);
	shader.use();
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer->pos));
	shader.setInt("rsm_position", 0);
	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer->albedo));
	shader.setInt("rsm_flux", 1);
	GLCall(glActiveTexture(GL_TEXTURE2));
	GLCall(glBindTexture(GL_TEXTURE_2D, rsm_buffer->normal));
	shader.setInt("rsm_normal", 2);
	GLCall(glBindImageTexture(0, samples_tbo, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F));
	GLCall(glDispatchCompute(num_vpls_first_bounce/16, 1, 1));
	GLCall(glMemoryBarrier(GL_ALL_BARRIER_BITS));
	ssbo_1st_vpls.unbind();
}

//DEBUG CODE
//{
//	deferred_lighting(shader_list.table["dlighting"], fsquad, scene_transforms, light_transforms, ldata, fblist["gbuffer"], fblist["shadowmap"]);
//}

//blit
//{
//	//gputimer a("blit");
//	debug_blit(shader_list.table["blit"], fsquad, fblist["layered_fbo"]->albedo, cur_Layer);
//}		

//std::vector<VPL> arr(num_vpls_first_bounce);
//ssbo_1st_vpls.bind();
//VPL* ptr = (VPL*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//memcpy(arr.data(), ptr, num_vpls_first_bounce * sizeof(VPL));
//glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
//auto a = 10;