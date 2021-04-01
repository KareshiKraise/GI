#pragma once

#include "window.h"
#include <GLFW/glfw3.h>
#include "mesh_loader.h"
#include "shader_table.h"
#include "framebuffer.h"
#include "camera.h"
#include "primitives.h"
#include "gputimer.h"
#include "SSBO.h"

struct light_props {
	glm::vec3 light_pos;
	glm::vec3 light_dir;
	glm::vec3 light_col;	
};

struct VPL {
	glm::vec4 pos;
	glm::vec4 normal;
	glm::vec4 color;	
};

using Transforms = std::unordered_map<std::string, glm::mat4>;

void generate_gbuffer(framebuffer* fbo, program& shader, Transforms& mats, mesh_loader& model);

void debug_blit(program& shader, quad& fsquad, framebuffer* layered, int cur_Layer);

void generate_rsm(framebuffer* fbo, program& shader, Transforms mats, mesh_loader& model, light_props& ldata);

void generate_shadowmap(framebuffer* fbo, program& shader, Transforms mats, mesh_loader& model, light_props& ldata);

void deferred_lighting(program& shader, quad& fsquad, Transforms& scene_transforms, Transforms& light_transforms, light_props& ldata, framebuffer* gbuffer, framebuffer* shadowmap, framebuffer* layered, int num_vpls_first_bounce, float pnear, float pfar, camera& c);

void generate_importance_map(program& shader, framebuffer* gbuffer, framebuffer* rsm, SSBO& brsm, SSBO& samples);

void generate_cdf_cols(program& shader, SSBO& brsm, SSBO& cdf_cols, float rsm_w, float rsm_h);

void generate_cdf_rows(program& shader, SSBO& cdf_cols, SSBO& cdf_rows, float rsm_w, float rsm_h);

void generate_vpls(program& shader, framebuffer* rsm, SSBO& ssbo_cdf_cols, SSBO& ssbo_cdf_rows, SSBO& ssbo_1st_vpls, int num_vpls_first_bounce, GLuint sample_cdf_tbo);

void generate_vals(program& shader, SSBO& ssbo_1st_vpls, SSBO& ssbo_vals, int num_vpls_first_bounce, int num_clusters);

void calc_distance_to_val(program& shader, int num_clusters, int num_vpls_first_bounce, bool first_pass, SSBO& ssbo_1st_vpls, SSBO& ssbo_vals, SSBO& vpl_to_vals, SSBO& vpl_count);

void update_cluster_centers(program& shader, int num_vpls_first_bounce, int num_clusters, SSBO& ssbo_1st_vpls, SSBO& ssbo_vals, SSBO& vpl_to_vals, SSBO& vpl_count);

void render_cluster_shadow_map(program& shader, framebuffer* layered_fbo, float near, float far, mesh_loader& model, int num_clusters, Transforms& transforms);

void propagate_vpls(program& shader, GLuint sample_texture, framebuffer* clusters_fbo, SSBO& sec_bounce_vpls, SSBO& sec_bounce_count, float radius, int num_clusters);

void update_light_buffers(program& shader, SSBO& ssbo_1st_vpls, SSBO& ssbo_vals, SSBO& vpl_to_vals, SSBO& vpl_count, int num_vpls_first_bounce, int num_clusters);

void split_gbuffer(program& shader, framebuffer* gbuffer, framebuffer* interleaved_buffer, int n_rows, int n_cols, int Wid, int Hei);

void shade_interleaved_buffer(program& shader, GLuint drawtex, framebuffer* interleaved_gbuffer, framebuffer* layered_gbuffer, framebuffer* shadowmap,
	framebuffer* rsm, Transforms& light_transforms, int ncols, int nrows, int num_vpls_first_bounce,
	int num_clusters, int num_vpls_second_bounce, float near, float far, camera& c, light_props& ldata, bool see_bounce, int wid, int hei, bool direct_only, float spotlight_cutoff=0.0f, bool is_spotlight = false);

void join_buffers(program& shader, GLuint interleaved_tex, GLuint final_tex, int num_rows, int num_cols, int Wid, int Hei);

void blit_to_screen(program& shader, framebuffer* gbuffer, GLuint radianceBuffer, float near, float far, quad& q);

void edge_detection(program& shader, framebuffer* gbuffer, GLuint edge_tex);

void blur_pass(program& xblur, program& yblur, int num_blur_pass, GLuint edge_tex, GLuint in_tex, GLuint out_tex, quad& q, float Wid, float Hei);

void sample_vpls_from_rsm(program& shader, framebuffer* rsm_buffer, SSBO& ssbo_1st_vpls, int num_vpls_first_bounce, GLuint samples_tbo);