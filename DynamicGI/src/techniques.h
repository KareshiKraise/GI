#pragma once
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "window.h"
#include "GL_CALL.h"
#include "mesh_loader.h"
#include "camera.h"
#include "shader.h"
#include "Quad.h"
#include "framebuffer.h"
#include "Axis3D.h"
#include "uniform_buffer.h"
#include <random>
#include "shader_storage_buffer.h"

#include "third_party/imgui.h"
#include "third_party/imgui_impl_glfw.h"
#include "third_party/imgui_impl_opengl3.h"

extern const float PI;
extern const float PI_TWO;
extern int VPL_SAMPLES;

//Number of turns for spiral quasi monte carlo sampling, pre computed by McGuire for max of N = 100 samples 
//Taken from the G3D DeepGBuffer radiosity samples
//https://casual-effects.com/g3d/www/index.html
static int minDiscrepancyArray[100] = {
	//  0   1   2   3   4   5   6   7   8   9
	1,  1,  1,  2,  3,  2,  5,  2,  3,  2,  // 0
	3,  3,  5,  5,  3,  4,  7,  5,  5,  7,  // 1
	9,  8,  5,  5,  7,  7,  7,  8,  5,  8,  // 2
	11, 12,  7, 10, 13,  8, 11,  8,  7, 14,  // 3
	11, 11, 13, 12, 13, 19, 17, 13, 11, 18,  // 4
	19, 11, 11, 14, 17, 21, 15, 16, 17, 18,  // 5
	13, 17, 11, 17, 19, 18, 25, 18, 19, 19,  // 6
	29, 21, 19, 27, 31, 29, 21, 18, 17, 29,  // 7
	31, 31, 23, 18, 25, 26, 25, 23, 19, 34,  // 8
	19, 27, 21, 25, 39, 29, 17, 21, 27, 29 }; // 9

//dachsbacher 2005 RSM (unused)
struct RSM_parameters {
	std::vector<glm::vec2> pattern;
	unsigned int SAMPLING_SIZE; // rsm sampling
	float r_max;// max r for sampling pattern of rsm
	float rsm_w;
	float rsm_h;
};

struct scene {
	mesh_loader *mesh;	
	Camera *camera;	
	glm::vec3 bb_mid;	
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 model;
	float n_val;
	float f_val;
};

struct shadow_data {
	glm::mat4 proj;
	glm::mat4 view;	
	glm::mat4 light_space_mat;
	Shader *shader;
	float s_w;
	float s_h;
	float s_near;
	float s_far;
	glm::vec3 lightDir;
	glm::vec3 lightColor;
	glm::vec3 lightPos;
};

struct point_light {
	glm::vec4 p; //position	 + radius
	glm::vec4 n;
	glm::vec4 c;//color	
};

struct frustum {
	glm::vec4 planes[4];
};

/* --- FUNCTION SIGNATURES ---*/

void cluster_vpls(Shader& cluster_program, const framebuffer& rsm_buffer, int num_VALs, int num_VPLs, bool start_frame, uniform_buffer& kvals);

void update_clusters(Shader& update_cluster_program, int num_VALs, int num_VPLs);

void rsm_pass(Shader& RSM_pass, framebuffer& rsm, shadow_data& light_data, scene& s);

void gbuffer_pass(framebuffer& gbuffer, Shader& gbuf_program, scene& s, glm::mat4& view);

void deep_g_buffer_pass(Shader& dgb_program, framebuffer& dgbuffer, scene& s, float delta);

void deep_g_buffer_debug(Shader& debug_view, framebuffer& dgb, quad& screen, shadow_data&  light_data, scene& s, framebuffer& depth_buffer);

void generate_tile_frustum(Shader& frustum_program, shader_storage_buffer& buffer, float Wid, float Hei, glm::mat4& invProj);

void shadow_pass(shadow_data& data, framebuffer& depth_buffer, mesh_loader& mesh, glm::mat4& model);

std::vector<glm::vec2> gen_uniform_samples(unsigned int s, float min, float max);

void compute_vpl_propagation(Shader& ssvp, const std::vector<glm::mat4>& pView, const framebuffer& gbackbuffer, const std::vector<framebuffer>& paraboloidmaps,
	GLuint samplesTBO, float near, float far, int NUM_VPLS, int num_VALs, float vpl_radius);

void do_SSVP(Shader& SSVP, shader_storage_buffer& lightSSBO, GLuint samplesTBO, const framebuffer& cubemap, glm::vec4 refPos, int NumLights, int MaxLights, unsigned int factor);

glm::mat4 axis_from_rotation(float yaw, float pitch, glm::vec3 eye = glm::vec3(0.0));

void fill_lightSSBO(const framebuffer& buffer, const glm::mat4& view, Shader& ssbo_program ,GLuint tbo, shader_storage_buffer& ssbo, int samples, float vpl_radius);

void draw_skybox(Shader& skybox_program, const framebuffer& cube_buf, glm::mat4 view, glm::mat4 proj, Cube& cube);

void generate_rsm_sampling_pattern(std::vector<glm::vec2>& p, RSM_parameters& rsm_param);

void blur_pass(quad& screen, unsigned int source_id, Shader& blur_program, framebuffer& blur_buffer);

void do_tiled_shading(Shader& tiled_shading, framebuffer& gbuffer, framebuffer& rsm_buffer, GLuint draw_tex, glm::mat4& invProj, scene& sponza, 
	shadow_data& shadowmap, int numVPL, int numVAL ,shader_storage_buffer& lightSSBO, float Wid, float Hei, 
	float near, float far, const std::vector<glm::mat4>& pView, const std::vector<framebuffer>& pfbo, bool see_bounce);

void do_parabolic_map(const glm::mat4& parabolicModelView, framebuffer& parabolic_sm, Shader& parabolic_map, float ism_w, float ism_h, scene& sponza);

void do_parabolic_rsm(const glm::mat4& parabolicModelView, framebuffer& parabolic_sm, Shader& parabolic_map, float ism_w, float ism_h, scene& sponza, float n, float f);

void generate_vals(Shader& gen_vals, const framebuffer& rsm_buffer, GLuint val_sample_tbo, int num_vpls, int num_clusters);

void calc_distance_to_val(Shader& clusterize_vals, int num_val_clusters, int num_vpls, bool first_pass);

void update_cluster_centers(Shader& update_vals, int num_vpls);